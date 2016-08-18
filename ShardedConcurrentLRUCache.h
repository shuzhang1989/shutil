//
// @author shu (shu@pinterest.com)
//
// A simple sharded LRU cache. Internally it is sharded to several lru cache
// shards to reduce contention.
// The total number of objects can be cached = shard_count * per_shard_limit
// Each shard is protected by a RWSpinLock.
// Usage:
// get() will return nullptr if the key is not stored.


#ifndef COMMON_CONCURRENT_LRU_CACHE_H_
#define COMMON_CONCURRENT_LRU_CACHE_H_

#include <functional>
#include <vector>

#include "folly/EvictingCacheMap.h"
#include "folly/RWSpinLock.h"

using folly::EvictingCacheMap;
using folly::RWSpinLock;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

namespace common {

template <class K, class V>
class ConcurrentLRUCache {
  // The cache shard is essentially a non-thread-safe EvictingCache
  // protected by a RWSpinLock
  struct LRUCacheShard {
    explicit LRUCacheShard(const size_t cap): cache_store_(cap) {}

    void put(const K& key, shared_ptr<V> value) {
      RWSpinLock::WriteHolder write_guard(rw_lock_);
      cache_store_.set(key, value);
    }

    shared_ptr<V> get(const K& key) {
      // For read, we also want to get a write lock.
      // Inside EvictingCacheMap, the get() will have some motificaiton
      // on the list, which cannot be protected by a read lock.
      RWSpinLock::WriteHolder write_guard(rw_lock_);
      if (!cache_store_.exists(key)) {
        return nullptr;
      }
      return cache_store_.get(key);
    }

    EvictingCacheMap<K, shared_ptr<V>> cache_store_;
    mutable RWSpinLock rw_lock_;
  };

 public:
  explicit ConcurrentLRUCache(
          const size_t shards_cap,
          const size_t per_shard_cap): shards_cap_(shards_cap) {
    lru_shards_.reserve(shards_cap);
    for (size_t i = 0; i < shards_cap; i++) {
      unique_ptr<LRUCacheShard> shard =
              std::make_unique<LRUCacheShard>(per_shard_cap);
      lru_shards_.emplace_back(std::move(shard));
    }
  }

  void put(const K& key, shared_ptr<V> value) {
    return lru_shards_[get_shard(key)]->put(key, value);
  }

  shared_ptr<V> get(const K& key) {
    return lru_shards_[get_shard(key)]->get(key);
  }

 private:
  size_t get_shard(const K& key) {
    return std::hash<K>{}(key) % shards_cap_;
  }

  vector<unique_ptr<LRUCacheShard>> lru_shards_;
  const size_t shards_cap_;
};

}  // namespace common

#endif  // COMMON_CONCURRENT_LRU_CACHE_H_
