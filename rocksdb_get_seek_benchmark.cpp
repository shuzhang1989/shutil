//
// Created by Shu Zhang on 9/7/16.
//

#include <folly/String.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include "glog/logging.h"
#include "folly/Benchmark.h"
#include "rocksdb/cache.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/backupable_db.h"
#include "rocksdb/env.h"
#include "rocksdb/write_batch.h"

using std::string;
using rocksdb::Status;

// RocksDB key schema:
// <key_id> + "H" + <object_id>
// Value: long string

rocksdb::DB* db_ptr;


rocksdb::DB* initializeRocksdbInstance(bool full_bloom) {
  rocksdb::Options options;
  rocksdb::BlockBasedTableOptions table_options;
  table_options.block_cache =
          rocksdb::NewLRUCache(100 * 1024 * 1024);
  // This caches seek indexes
  //table_options.cache_index_and_filter_blocks = true;
  if (full_bloom) {
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(
            10, false));
  } else {
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(
            10, true));
  }
  options.table_factory
        .reset(rocksdb::NewBlockBasedTableFactory(table_options));
  options.create_if_missing = true;
  rocksdb::DB* db;
  Status s = rocksdb::DB::Open(options, "test_benchmark_db", &db);
  if (!s.ok()) {
    LOG(ERROR) << "Failed to create rocksdb instance:" << s.ToString();
    return nullptr;
  }
  // Initialize and input key_id from 0 to 100,000,000 and correspondingly
  // object id 0 to 10 for each key
  // value is string called "hello"
  /*
  rocksdb::WriteBatch batch;
  for (int i = 0; i < 1000000; i ++) {
    for (int j = 0; j < 10; j ++) {
      auto key = folly::stringPrintf("%010d_%010d", i, j);
      batch.Put(key, "Hello world");
    }
  }
  s = db->Write(rocksdb::WriteOptions(), &batch);
  if (!s.ok()) {
    LOG(ERROR) << "Failed to write to db";
    return nullptr;
  }
  */
  return db;
}

BENCHMARK(GetBenchmark, n) {
  rocksdb::ReadOptions options;
  while (n--) {
    // random a key
    auto key = folly::stringPrintf("%010d_%010d", rand() % 1000000, rand() %
            10);
    std::string value;
    rocksdb::Status s = db_ptr->Get(rocksdb::ReadOptions(), key, &value);
    if (value != "Hello world") {
      std::cout << "Error!" << std::endl;
      exit(0);
    }
  }
}


BENCHMARK(SeekBenchmarkNonFullBloom, n) {
  rocksdb::ReadOptions options;
  while (n--) {
    // random a key
    rocksdb::Iterator* it = db_ptr->NewIterator(rocksdb::ReadOptions());
    auto key = folly::stringPrintf("%010d_%010d", rand() % 1000000, rand() %
            10);
    it->Seek(key);
    if (it->value().ToString() != "Hello world") {
      std::cout << "Error!" << std::endl;;
      exit(0);
    }
  }
}


int main(int argc, char** argv) {
  db_ptr = initializeRocksdbInstance(false);
  if (!db_ptr) {
    rocksdb::Options options;
    rocksdb::DestroyDB("test_benchmark_db", options);
    return -1;
  }
  folly::runBenchmarks();
  return 0;
}
