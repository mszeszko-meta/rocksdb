//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include "port/port.h"
#include "rocksdb/rocksdb_namespace.h"
#include "util/channel.h"
#include "work_items.h"

namespace ROCKSDB_NAMESPACE {

class ThreadPool {
 public:
  ThreadPool(std::string name) : name_(std::move(name)){};
  virtual ~ThreadPool();

  void Initialize(int num_threads,
                  CpuPriority threads_cpu_priority = CpuPriority::kNormal);

  void AddWorkItem(WorkItem& work_item);
  CpuPriority GetCpuPriority();
  void SetCpuPriority(CpuPriority threads_cpu_priority);
  void UpdateMaxThreads(int num_threads);

  virtual void DoWork(WorkItem& work_item, WorkItemResult& result) = 0;
 private:
  const std::string name_;
  mutable channel<WorkItem> work_items_;
  std::vector<port::Thread> threads_;
  std::atomic<int> num_threads_;
  std::atomic<CpuPriority> threads_cpu_priority_;
};

}  // namespace ROCKSDB_NAMESPACE
