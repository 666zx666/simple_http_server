/**
 * @file thread_pool.h
 * @author zX
 * @brief Thread Pool
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "work_thread.h"
#include <semaphore.h>//使用线程信号量
#include "parameters.h"
#include "work_queue.h"

namespace http_server
{

/*@brief 线程池类*/
class ThreadPool : public boost::noncopyable
{
  typedef std::function<void()> TaskFunc;

public:
  ThreadPool(parameters::Parameters *pool_parameters);

  void start();

  void thread_routine(int index);

  void distribute_task();

  work_thread::WorkThread *get_next_work_thread();

  ~ThreadPool()
  {
    for (int i = 0; i < work_threads_.size(); ++i)
    {
      delete work_threads_[i];
    }
  }

  status add_task_to_pool(TaskFunc new_task);

  void close_pool();

private:
  std::vector<work_thread::WorkThread *> work_threads_;//工作线程存储数组vector
  std::shared_ptr<my_thread::Thread> distribute_thread_;//分发线程
  int next_;//标记下一个工作线程
  int threads_num_;//线程数
  bool started;//线程池开启标志位
  my_mutex::MutexLock boot_mutex_;//线程池启动锁
  my_condition::Condition boot_cond_;//线程池启动条件

  pthread_barrier_t pool_barrier_;//线程池屏障
  bool pool_activate;//线程池激活标志位

  my_mutex::MutexLock pool_mutex_;//线程池锁

  sem_t task_num_;//任务信号量

  WorkQueue<work_thread::Work::WorkPtr> pool_work_queue_;//线程池工作队列

  parameters::Parameters *pool_parameters_;

  int max_work_num_;
};

} // namespace http_server

#endif // THREAD_POOL_H_