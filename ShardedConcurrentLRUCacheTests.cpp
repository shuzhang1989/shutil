// Copyright 2009-present Pinterest. All Rights Reserved.
//
// @author shu (shu@pinterest.com)
//

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "common/concurrent_lru_cache.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

using common::ConcurrentLRUCache;
using std::string;

TEST(ConcurrentLRUCache, Basics) {
  // Single threaded. No eviction.
  ConcurrentLRUCache<string, string> cache =
          ConcurrentLRUCache<string, string>(2, 5);
  auto value = std::make_shared<string>("a");
  cache.put("a", value);
  EXPECT_EQ(*cache.get("a"), "a");
  EXPECT_EQ(cache.get("b"), nullptr);
}

TEST(ConcurrentLRUCache, Eviction) {
  // Single threaded, eviction.
  ConcurrentLRUCache<string, string> cache =
          ConcurrentLRUCache<string, string>(1, 2);
  auto evicting_value = std::make_shared<string>("a");
  cache.put("a", evicting_value);
  cache.put("b", std::make_shared<string>("b"));
  cache.put("c", std::make_shared<string>("c"));
  EXPECT_EQ(cache.get("a"), nullptr);
  EXPECT_EQ(*cache.get("b"), "b");
  EXPECT_EQ(*cache.get("c"), "c");
  EXPECT_EQ(*evicting_value, "a");
}

TEST(ConcurrentLRUCache, StressNoEvict) {
  // capacity of 4 with 10 threads, no eviction.
  ConcurrentLRUCache<string, string> cache =
          ConcurrentLRUCache<string, string>(2, 4);
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; i++) {
    threads.push_back(std::thread([&cache]() {
      for (int j = 0; j < 100000; j ++) {
        cache.put("a", std::make_shared<string>("a"));
        cache.put("b", std::make_shared<string>("b"));
        cache.put("c", std::make_shared<string>("c"));
        cache.put("d", std::make_shared<string>("d"));
        EXPECT_EQ(*cache.get("a"), "a");
        EXPECT_EQ(*cache.get("b"), "b");
        EXPECT_EQ(*cache.get("c"), "c");
        EXPECT_EQ(*cache.get("d"), "d");
      }
    }));
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

void simulate_get_put(ConcurrentLRUCache<string, string>* cache,
        const string& key, int* miss_count, int* hit_count) {
  auto value_ptr = cache->get(key);
  if (value_ptr == nullptr) {
    (*miss_count)++;
    auto new_value = std::make_shared<string>(key);
    cache->put(key, new_value);
  } else {
    EXPECT_EQ(*value_ptr, key);
    (*hit_count)++;
  }
}

TEST(ConcurrentLRUCache, StressEvict) {
  ConcurrentLRUCache<string, string> cache =
          ConcurrentLRUCache<string, string>(2, 4);
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; i++) {
    threads.push_back(std::thread([&cache]() {
      int hit_count = 0;
      int miss_count = 0;
      for (int j = 0; j < 100000; j++) {
        simulate_get_put(&cache, "a", &hit_count, &miss_count);
        simulate_get_put(&cache, "b", &hit_count, &miss_count);
        simulate_get_put(&cache, "c", &hit_count, &miss_count);
        simulate_get_put(&cache, "d", &hit_count, &miss_count);
        simulate_get_put(&cache, "e", &hit_count, &miss_count);
        simulate_get_put(&cache, "f", &hit_count, &miss_count);
        simulate_get_put(&cache, "g", &hit_count, &miss_count);
      }
      LOG(INFO) << "Hitting count:" << hit_count;
      LOG(INFO) << "Missing count:" << miss_count;
      EXPECT_EQ(hit_count + miss_count, 700000);
    }));
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;
  return RUN_ALL_TESTS();
}
