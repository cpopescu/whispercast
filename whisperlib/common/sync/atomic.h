// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

#ifndef __COMMON_SYNC_ATOMIC_H__
#define __COMMON_SYNC_ATOMIC_H__

namespace synch {

// Atomically perform the operation suggested by the name, and returns the
// value that had previously been in memory.

template <typename T>
T AtomicFetchAndAdd(T* ptr, T value) {
  return __sync_fetch_and_add(ptr, value);
}
template <typename T>
T AtomicFetchAndSub(T* ptr, T value) {
  return __sync_fetch_and_sub(ptr, value);
}

// Atomically perform the operation suggested by the name, and return
// the new value.

template <typename T>
T AtomicAddAndFetch(T* ptr, T value) {
  return __sync_add_and_fetch(ptr, value);
}
template <typename T>
T AtomicSubAndFetch(T* ptr, T value) {
  return __sync_sub_and_fetch(ptr, value);
}

// Atomic compare and swap: if the current value of *ptr is oldval,
// then write newval into *ptr.

// returns true if the comparison is successful and newval was written.
template <typename T>
T AtomicCompareAndSwap(T* ptr, T oldval, T newval) {
  return __sync_bool_compare_and_swap(ptr, oldval, newval);
}

}

#endif // __COMMON_SYNC_ATOMIC_H__
