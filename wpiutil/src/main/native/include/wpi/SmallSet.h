//===- llvm/ADT/SmallSet.h - 'Normally small' sets --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the SmallSet class.
//
//===----------------------------------------------------------------------===//

#ifndef WPIUTIL_WPI_SMALLSET_H
#define WPIUTIL_WPI_SMALLSET_H

#include "wpi/None.h"
#include "wpi/SmallPtrSet.h"
#include "wpi/SmallVector.h"
#include "wpi/Compiler.h"
#include <cstddef>
#include <functional>
#include <set>
#include <utility>

namespace wpi {

/// SmallSet - This maintains a set of unique values, optimizing for the case
/// when the set is small (less than N).  In this case, the set can be
/// maintained with no mallocs.  If the set gets large, we expand to using an
/// std::set to maintain reasonable lookup times.
///
/// Note that this set does not provide a way to iterate over members in the
/// set.
template <typename T, unsigned N, typename C = std::less<T>>
class SmallSet {
  /// Use a SmallVector to hold the elements here (even though it will never
  /// reach its 'large' stage) to avoid calling the default ctors of elements
  /// we will never use.
  SmallVector<T, N> Vector;
  std::set<T, C> Set;

  using VIterator = typename SmallVector<T, N>::const_iterator;
  using mutable_iterator = typename SmallVector<T, N>::iterator;

  // In small mode SmallPtrSet uses linear search for the elements, so it is
  // not a good idea to choose this value too high. You may consider using a
  // DenseSet<> instead if you expect many elements in the set.
  static_assert(N <= 32, "N should be small");

public:
  using size_type = size_t;

  SmallSet() = default;

  LLVM_NODISCARD bool empty() const {
    return Vector.empty() && Set.empty();
  }

  size_type size() const {
    return isSmall() ? Vector.size() : Set.size();
  }

  /// count - Return 1 if the element is in the set, 0 otherwise.
  size_type count(const T &V) const {
    if (isSmall()) {
      // Since the collection is small, just do a linear search.
      return vfind(V) == Vector.end() ? 0 : 1;
    } else {
      return Set.count(V);
    }
  }

  /// insert - Insert an element into the set if it isn't already there.
  /// Returns true if the element is inserted (it was not in the set before).
  /// The first value of the returned pair is unused and provided for
  /// partial compatibility with the standard library self-associative container
  /// concept.
  // FIXME: Add iterators that abstract over the small and large form, and then
  // return those here.
  std::pair<NoneType, bool> insert(const T &V) {
    if (!isSmall())
      return std::make_pair(None, Set.insert(V).second);

    VIterator I = vfind(V);
    if (I != Vector.end())    // Don't reinsert if it already exists.
      return std::make_pair(None, false);
    if (Vector.size() < N) {
      Vector.push_back(V);
      return std::make_pair(None, true);
    }

    // Otherwise, grow from vector to set.
    while (!Vector.empty()) {
      Set.insert(Vector.back());
      Vector.pop_back();
    }
    Set.insert(V);
    return std::make_pair(None, true);
  }

  template <typename IterT>
  void insert(IterT I, IterT E) {
    for (; I != E; ++I)
      insert(*I);
  }

  bool erase(const T &V) {
    if (!isSmall())
      return Set.erase(V);
    for (mutable_iterator I = Vector.begin(), E = Vector.end(); I != E; ++I)
      if (*I == V) {
        Vector.erase(I);
        return true;
      }
    return false;
  }

  void clear() {
    Vector.clear();
    Set.clear();
  }

private:
  bool isSmall() const { return Set.empty(); }

  VIterator vfind(const T &V) const {
    for (VIterator I = Vector.begin(), E = Vector.end(); I != E; ++I)
      if (*I == V)
        return I;
    return Vector.end();
  }
};

/// If this set is of pointer values, transparently switch over to using
/// SmallPtrSet for performance.
template <typename PointeeType, unsigned N>
class SmallSet<PointeeType*, N> : public SmallPtrSet<PointeeType*, N> {};

} // end namespace wpi

#endif // LLVM_ADT_SMALLSET_H
