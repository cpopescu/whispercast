// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Cosmin Tudorache

#ifndef __COMMON_BASE_CACHE_H__
#define __COMMON_BASE_CACHE_H__

#include <map>
#include <list>
#include <vector>
//#include <ios>
#include <iostream>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/sync/mutex.h>

namespace util {

template <typename V>
void DefaultValueDestructor(V v) {
  delete v;
};

struct CacheBase {
  // replacement algorithm
  enum Algorithm {
    LRU,
    MRU,
    RANDOM,
  };
  static const char* AlgorithmName(Algorithm alg) {
    switch(alg) {
      CONSIDER(LRU);
      CONSIDER(MRU);
      CONSIDER(RANDOM);
    }
    LOG_FATAL << "Illegal Algorithm: " << alg;
    return "Unknown";
  }
};

template <typename K, typename V>
class Cache : public CacheBase {
public:
  typedef void (*ValueDestructor)(V);

private:
  class Item;
  typedef map<K, Item*> ItemMap;
  typedef map<uint64, Item*> ItemByUseMap;
  typedef list<Item*> ItemList;
  typedef typename ItemMap::iterator ItemMapIterator;
  typedef typename ItemList::iterator ItemListIterator;

private:
  class Item {
  public:
    Item(V value, ValueDestructor destructor, uint64 use, int64 expiration_ts)
      : value_(value), destructor_(destructor), use_(use),
        expiration_ts_(expiration_ts), items_it_(), items_by_exp_it_() {
    }
    ~Item() {
      if ( destructor_ != NULL ) {
        (*destructor_)(value_);
      }
    }
    V value() const { return value_; }
    uint64 use() const { return use_; }
    int64 expiration_ts() const { return expiration_ts_; }
    ItemMapIterator items_it() { return items_it_; }
    ItemListIterator items_by_exp_it() { return items_by_exp_it_; }
    void set_value(V value) { value_ = value; }
    void set_use(uint64 use) { use_ = use; }
    void set_items_it(ItemMapIterator it) { items_it_ = it; }
    void set_items_by_exp_it(ItemListIterator it) { items_by_exp_it_ = it; }
    K key() { return items_it_->first; }
    string ToString() const {
      ostringstream oss;
      oss << "(use_: " << use_ << ", value_: " << value_
          << ", expiration_ts_: " << expiration_ts_ << ")";
      return oss.str();
    }
  private:
    const V value_;
    // this function is used to delete the 'value_' in destructor
    const ValueDestructor destructor_;
    // a value which increases every time this Item is used. Useful for LRU/MRU.
    uint64 use_;
    // expiration time stamp (ms by CLOCK_MONOTONIC)
    const int64 expiration_ts_;
    // points to the place of this item inside 'Cache::items_' map
    ItemMapIterator items_it_;
    // points to the place of this item inside 'Cache::items_by_exp_' list
    ItemListIterator items_by_exp_it_;
  };


public:
  // destructor: a function which knows how to delete V type
  //             Use &DefaultValueDestructor() for default "delete" operator.
  //             Use NULL is you don't wish to destroy the values.
  // null_value: what the cache returns on Get() if the key is not found.
  // synchornized: if true, the cache is synchronized with a mutex
  Cache(Algorithm algorithm, uint32 max_size, uint64 expiration_time_ms,
        ValueDestructor destructor, V null_value, bool synchornized)
    : CacheBase(),
      algorithm_(algorithm),
      max_size_(max_size),
      expiration_time_ms_(expiration_time_ms),
      destructor_(destructor),
      null_value_(null_value),
      items_(),
      items_by_use_(),
      items_by_exp_(),
      next_use_(0),
      sync_(synchornized ? new synch::Mutex(true) : NULL) {
  }
  virtual ~Cache() {
    Clear();
    delete sync_;
    sync_ = NULL;
  }

  Algorithm algorithm() const {
    return algorithm_;
  }
  const char* algorithm_name() const {
    return AlgorithmName(algorithm());
  }

  uint32 Size() const {
    synch::MutexLocker lock(sync_);
    return items_.size();
  }

  bool Add(K key, V value, bool replace = true) {
    synch::MutexLocker lock(sync_);
    ExpireSomeCache();
    Item* old_item = Find(key);
    if ( old_item != NULL ) {
      if ( !replace ) {
        return false;
      }
      DelItem(old_item);
      old_item = NULL;
    }
    if ( items_.size() >= max_size_ ) {
      DelByAlgorithm();
    }
    uint64 use = NextUse();
    Item* item = new Item(value, destructor_, use,
        timer::TicksMsec() + expiration_time_ms_);
    pair<ItemMapIterator, bool> result = items_.insert(make_pair(key, item));
    item->set_items_it(result.first);
    items_by_exp_.push_back(item);
    item->set_items_by_exp_it(--items_by_exp_.end());
    items_by_use_[use] = item;
    return true;
  }
  V Get(K key) {
    synch::MutexLocker lock(sync_);
    ExpireSomeCache();
    Item* item = Find(key);
    if ( item == NULL ) {
      return null_value_;
    }
    items_by_use_.erase(item->use());
    item->set_use(NextUse());
    items_by_use_[item->use()] = item;
    return item->value();
  }
  void GetAll(map<K,V>* out) {
    synch::MutexLocker lock(sync_);
    ExpireSomeCache();
    for ( typename ItemMap::iterator it = items_.begin(); it != items_.end();
          ++it ) {
      const K& key = it->first;
      const Item* item = it->second;
      out->insert(make_pair(key, item->value()));
    }
  }
  // This method exists only for K == string.
  V GetPathBased(K key) {
    synch::MutexLocker lock(sync_);
    Item* item = io::FindPathBased(&items_, key);
    if ( item == NULL ) {
      return null_value_;
    }
    return item->value();
  }
  // removes a value from the cache, and returns it.
  V Pop(K key) {
    synch::MutexLocker lock(sync_);
    ExpireSomeCache();
    Item* item = Find(key);
    if ( item == NULL ) {
      return null_value_;
    }
    V v = item->value();
    item->set_data(null_value_);
    Del(item);
    return v;
  }
  void Del(K key) {
    synch::MutexLocker lock(sync_);
    Item* item = Find(key);
    if ( item == NULL ) {
      return;
    }
    DelItem(item);
  }

  void Clear() {
    synch::MutexLocker lock(sync_);
    for ( typename ItemMap::iterator it = items_.begin();
          it != items_.end(); ++it ) {
      Item* item = it->second;
      delete item;
    }
    items_.clear();
    items_by_use_.clear();
  }

  string ToString() const {
    synch::MutexLocker lock(sync_);
    ostringstream oss;
    oss << "Cache{algorithm_: " << algorithm_name()
        << ", max_size_: " << max_size_
        << ", items: ";
    for ( typename ItemMap::const_iterator it = items_.begin();
          it != items_.end(); ++it ) {
      const K& key = it->first;
      const Item* item = it->second;
      oss << endl << key << " -> " << item->ToString();
    }
    oss << "}";
    return oss.str();
  }

private:
  uint64 NextUse() {
    return next_use_++;
  }
  Item* Find(K key) {
    typename ItemMap::iterator it = items_.find(key);
    return it == items_.end() ? NULL : it->second;
  }
  void DelItem(Item* item) {
    items_.erase(item->items_it());
    items_by_use_.erase(item->use());
    items_by_exp_.erase(item->items_by_exp_it());
    delete item;
  }
  void DelByAlgorithm() {
    Item* to_del = NULL;
    if ( items_by_use_.empty() ) {
      return;
    }
    switch ( algorithm_ ) {
      case LRU:
        to_del = items_by_use_.begin()->second;
        break;
      case MRU:
        to_del = (--items_by_use_.end())->second;
        break;
      case RANDOM:
        to_del = items_.begin()->second;
        break;
    };
    DelItem(to_del);
  }
  void ExpireSomeCache() {
    int64 now = timer::TicksMsec();
    vector<Item*> to_del;
    for ( typename ItemList::iterator it = items_by_exp_.begin();
          it != items_by_exp_.end(); ++it ) {
      Item* item = *it;
      if ( item->expiration_ts() > now ) {
        // first item which expires in the future
        break;
      }
      to_del.push_back(item);
    }
    for ( uint32 i = 0; i < to_del.size(); i++ ) {
      DelItem(to_del[i]);
    }
  }

protected:
  const Algorithm algorithm_;
  const uint32 max_size_;
  const uint64 expiration_time_ms_;
  ValueDestructor destructor_;
  V null_value_;

  // a map of Item* by key.
  ItemMap items_;

  // a map of Item* by use_ count. Useful in LRU/MRU.
  ItemByUseMap items_by_use_;

  // a list of Item* by expiration time.
  ItemList items_by_exp_;

  // used for generating unique USE numbers
  // If the cache is used by 1000000 ops/sec, there will still be
  // 599730 years until this counter resets.
  uint64 next_use_;

  // if not NULL then all public methods are synchronized with this mutex
  synch::Mutex* sync_;
};

}; // namespace util

#endif   // __COMMON_BASE_CACHE_H__
