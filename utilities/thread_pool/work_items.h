//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <future>

#include "rocksdb/env.h"
#include "rocksdb/file_checksum.h"
#include "rocksdb/io_status.h"
#include "rocksdb/statistics.h"
#include "rocksdb/types.h"

namespace ROCKSDB_NAMESPACE {

struct WorkItemResult {
  WorkItemResult()
      : size(0),
        expected_src_temperature(Temperature::kUnknown),
        current_src_temperature(Temperature::kUnknown) {}

  WorkItemResult(const WorkItemResult& other) = delete;
  WorkItemResult& operator=(const WorkItemResult& other) = delete;

  WorkItemResult(WorkItemResult&& o) noexcept { *this = std::move(o); }

  WorkItemResult& operator=(WorkItemResult&& o) noexcept {
    size = o.size;
    checksum_hex = std::move(o.checksum_hex);
    db_id = std::move(o.db_id);
    db_session_id = std::move(o.db_session_id);
    io_status = std::move(o.io_status);
    expected_src_temperature = o.expected_src_temperature;
    current_src_temperature = o.current_src_temperature;
    return *this;
  }

  ~WorkItemResult() {
    // The Status needs to be ignored here for two reasons.
    // First, if the BackupEngineImpl shuts down with jobs outstanding, then
    // it is possible that the Status in the future/promise is never read,
    // resulting in an unchecked Status. Second, if there are items in the
    // channel when the BackupEngineImpl is shutdown, these will also have
    // Status that have not been checked.  This
    // TODO: Fix those issues so that the Status
    io_status.PermitUncheckedError();
  }
  uint64_t size;
  std::string checksum_hex;
  std::string db_id;
  std::string db_session_id;
  IOStatus io_status;
  Temperature expected_src_temperature = Temperature::kUnknown;
  Temperature current_src_temperature = Temperature::kUnknown;
};

enum WorkItemType : uint64_t {
  CopyOrCreate = 1U,
};

// Exactly one of src_path and contents must be non-empty. If src_path is
// non-empty, the file is copied from this pathname. Otherwise, if contents is
// non-empty, the file will be created at dst_path with these contents.
struct WorkItem {
  std::string src_path;
  std::string dst_path;
  Temperature src_temperature;
  Temperature dst_temperature;
  std::string contents;
  Env* src_env;
  Env* dst_env;
  EnvOptions src_env_options;
  bool sync;
  RateLimiter* rate_limiter;
  uint64_t size_limit;
  Statistics* stats;
  std::promise<WorkItemResult> result;
  std::function<void()> progress_callback;
  std::string src_checksum_func_name;
  std::string src_checksum_hex;
  std::string db_id;
  std::string db_session_id;
  WorkItemType type;

  WorkItem()
      : src_temperature(Temperature::kUnknown),
        dst_temperature(Temperature::kUnknown),
        src_env(nullptr),
        dst_env(nullptr),
        src_env_options(),
        sync(false),
        rate_limiter(nullptr),
        size_limit(0),
        stats(nullptr),
        src_checksum_func_name(kUnknownFileChecksumFuncName),
        type(WorkItemType::CopyOrCreate) {}

  WorkItem(const WorkItem&) = delete;
  WorkItem& operator=(const WorkItem&) = delete;

  WorkItem(WorkItem&& o) noexcept { *this = std::move(o); }

  WorkItem& operator=(WorkItem&& o) noexcept {
    src_path = std::move(o.src_path);
    dst_path = std::move(o.dst_path);
    src_temperature = std::move(o.src_temperature);
    dst_temperature = std::move(o.dst_temperature);
    contents = std::move(o.contents);
    src_env = o.src_env;
    dst_env = o.dst_env;
    src_env_options = std::move(o.src_env_options);
    sync = o.sync;
    rate_limiter = o.rate_limiter;
    size_limit = o.size_limit;
    stats = o.stats;
    result = std::move(o.result);
    progress_callback = std::move(o.progress_callback);
    src_checksum_func_name = std::move(o.src_checksum_func_name);
    src_checksum_hex = std::move(o.src_checksum_hex);
    db_id = std::move(o.db_id);
    db_session_id = std::move(o.db_session_id);
    src_temperature = o.src_temperature;
    type = std::move(o.type);
    return *this;
  }

  WorkItem(
      std::string _src_path, std::string _dst_path,
      const Temperature _src_temperature, const Temperature _dst_temperature,
      std::string _contents, Env* _src_env, Env* _dst_env,
      EnvOptions _src_env_options, bool _sync, RateLimiter* _rate_limiter,
      uint64_t _size_limit, Statistics* _stats, WorkItemType _type,
      std::function<void()> _progress_callback = {},
      const std::string& _src_checksum_func_name = kUnknownFileChecksumFuncName,
      const std::string& _src_checksum_hex = "", const std::string& _db_id = "",
      const std::string& _db_session_id = "")
      : src_path(std::move(_src_path)),
        dst_path(std::move(_dst_path)),
        src_temperature(_src_temperature),
        dst_temperature(_dst_temperature),
        contents(std::move(_contents)),
        src_env(_src_env),
        dst_env(_dst_env),
        src_env_options(std::move(_src_env_options)),
        sync(_sync),
        rate_limiter(_rate_limiter),
        size_limit(_size_limit),
        stats(_stats),
        progress_callback(_progress_callback),
        src_checksum_func_name(_src_checksum_func_name),
        src_checksum_hex(_src_checksum_hex),
        db_id(_db_id),
        db_session_id(_db_session_id),
        type(_type) {}

  ~WorkItem() = default;
};

}  // namespace ROCKSDB_NAMESPACE
