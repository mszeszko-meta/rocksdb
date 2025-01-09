//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "thread_pool.h"

#include "rocksdb/rocksdb_namespace.h"
#include "test_util/sync_point.h"
#include "work_items.h"

namespace ROCKSDB_NAMESPACE {

CpuPriority ThreadPool::GetCpuPriority() { return threads_cpu_priority_; }

void ThreadPool::SetCpuPriority(CpuPriority priority) {
  threads_cpu_priority_.store(priority);
}

void ThreadPool::AddWorkItem(WorkItem& work_item) {
  work_items_.write(std::move(work_item));
}

void ThreadPool::Initialize(int num_threads, CpuPriority threads_cpu_priority) {
  num_threads_.store(num_threads);
  threads_cpu_priority_.store(threads_cpu_priority);

  threads_.reserve(num_threads_);
  for (int t = 0; t < num_threads_; t++) {
    threads_.emplace_back([this]() {
#if defined(_GNU_SOURCE) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
      pthread_setname_np(pthread_self(), name_.c_str());
#endif
#endif
      CpuPriority current_priority = CpuPriority::kNormal;
      WorkItem work_item;
      while (work_items_.read(work_item)) {
        CpuPriority priority = threads_cpu_priority_;
        if (current_priority != priority) {
          TEST_SYNC_POINT_CALLBACK("ThreadPool::Initialize:SetCpuPriority",
                                   &priority);
          port::SetCpuPriority(0, priority);
          current_priority = priority;
        }
        WorkItemResult result;

        DoWork(work_item, result);

        work_item.result.set_value(std::move(result));
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  work_items_.sendEof();
  for (auto& thread : threads_) {
    thread.join();
  }
}

}  // namespace ROCKSDB_NAMESPACE
