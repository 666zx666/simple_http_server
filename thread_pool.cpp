/**
 * @file thread_pool.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "thread_pool.h"
#include <signal.h>
#include "work_queue.h"
//#define LOGGER_DEBUG
#define LOGGER_WARN
#include <logger.h>

namespace http_server
{

static struct timeval delay = {0, 2}; //延迟2微妙

ThreadPool::ThreadPool(parameters::Parameters *pool_parameters)
  : next_(0),
    threads_num_(0),
    started(false),
    boot_mutex_(),
    boot_cond_(boot_mutex_),
    pool_activate(false),
    pool_parameters_(pool_parameters)
{
  threads_num_ = pool_parameters_->getInitWorkerNum();
  max_work_num_ = pool_parameters_->getMaxWorkNum();
}

/**
 * @brief 启动线程池。设置工作线程。
 * 
 */
void ThreadPool::start()
{
  assert(!started);//assert的作用是先计算传入表达式，如果其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行。
  assert(threads_num_ != 0);

  sem_init(&task_num_, 0, 0);//线程信号量初始化

  started = true;

  pthread_barrier_init(&pool_barrier_, NULL, threads_num_ + 1); // 初始化线程屏障，屏障等待的最大线程数目为threads_num_ + 1（使用线程屏障使所有线程同步）。

  for(int i = 0; i < threads_num_; ++i)
  {
    work_thread::WorkThread* t = new work_thread::WorkThread(std::bind(&ThreadPool::thread_routine, this, i));
    work_threads_.push_back(t);
    t->state_ = BOOTING;
    t->start();
    boot_cond_.wait();//等待该线程开启
    t->state_ = READY;
  }

  pool_activate = true;

  distribute_thread_ = std::shared_ptr<my_thread::Thread>(new my_thread::Thread(std::bind(&ThreadPool::distribute_task, this)));
  distribute_thread_->start();

  pthread_barrier_wait(&pool_barrier_);//等待并检查线程屏障是否使所有线程同步，若是则执行下一句代码，否则线程继续等待

  INFO("Thread pool is ready to work.\n");  
}

/**
 * @brief 获得工作线程储存数组中下一个工作线程
 * 
 * @return work_thread::WorkThread* 
 */
work_thread::WorkThread* ThreadPool::get_next_work_thread()
{
  work_thread::WorkThread* next_work_thread;
  next_work_thread = work_threads_[next_];
  ++next_;
  if(static_cast<size_t>(next_) >= work_threads_.size())
  {
    next_ = 0;
  }
  return next_work_thread;
}

/**
 * @brief 工作线程例行函数。从工作队列中获取并执行工作.
 * 
 * @param 工作线程储存vector的索引 index 
 */
void ThreadPool::thread_routine(int index)
{
  pthread_detach(pthread_self());//分离当前线程，将状态改为unjoinable状态，使该线程函数退出时或pthread_exit时自动会被释放。
  INFO("Generated a work thread. thread id: %lu\n", pthread_self())//显示当前线程的ID

  assert(index < work_threads_.size());
  work_thread::WorkThread* this_work_thread = work_threads_[index];

  while(this_work_thread->state_ == BOOTING)
  {
    //my_mutex::MutexLockGuard mlg(boot_mutex_);
    boot_cond_.notify();//通知该线程开启
  }

  pthread_barrier_wait(&pool_barrier_);

  while(pool_activate)
  {
    if(this_work_thread->work_empty())
    {
      {
        my_mutex::MutexLockGuard mlg(this_work_thread->get_mutex());
        this_work_thread->state_ = IDLE;
      }
      this_work_thread->get_condition().wait();// 等待激活该工作线程
    }
    if(pool_activate == false)
      break;
    while(!this_work_thread->work_empty())
    {
      DEBUG("get a job. thread id: %lu\n", pthread_self());
      (this_work_thread->pop_work())->execute_work();//处理工作队列队首的工作
    }
  }
  INFO("Work thread %d exits.\n", index + 1);
}

/**
 * @brief 工作分发线程分配线程池工作队列中的工作任务。
 * 
 */
void ThreadPool::distribute_task()
{
  pthread_detach(pthread_self());
  INFO("distribute_task function started. thread id: %lu\n", pthread_self())

  while(pool_activate)
  {
    sem_wait(&task_num_);//线程信号量等待函数，每次使信号量值减1，直至为0
    if(pool_activate != true)
      break;
    work_thread::WorkThread* selected_thread = get_next_work_thread();
    assert(!pool_work_queue_.empty());
    work_thread::Work::WorkPtr work_to_past = pool_work_queue_.pop_work();
    selected_thread->add_work(work_to_past);
    {
      //my_mutex::MutexLockGuard mlg(selected_thread->get_mutex());
      if(selected_thread->state_ == IDLE)
      {
        selected_thread->state_ = BUSY;
        selected_thread->get_condition().notify();  //通知激活线程来执行这项工作
      }
    }
  }
  INFO("Distrubute task thread exits.\n");
}

/**
 * @brief 添加工作任务至线程池工作队列。 
 * 
 * @param new_task 
 * @return status 
 */
status ThreadPool::add_task_to_pool(TaskFunc new_task)
{
  if(pool_work_queue_.size() > max_work_num_)
  {
    WARN("Thread pool is busy. queue size: %d\n", pool_work_queue_.size());
    return FAILED;
  }
  std::shared_ptr<work_thread::Work> new_work = work_thread::Work::create_work(new_task);
  pool_work_queue_.push_work(new_work);
  //my_mutex::MutexLockGuard mlg(pool_mutex_);
  sem_post(&task_num_);//线程信号量增加1
  return SUCCESS;
}


/**
 * @brief 关闭线程池。
 * 
 */
void ThreadPool::close_pool()
{
  pool_activate = false;
  sem_post(&task_num_);//线程信号量增加1，当有线程等待这个信号量的时候等待的线程返回
  while(pthread_kill(distribute_thread_->thread_id(), 0) == 0)  // 确认已经退出线程。pthread_kill函数的参数signo为0时测试线程是否存在，返回0则存在
  {
    select(0, NULL, NULL, NULL, &delay);//定时等待
  }
  sem_destroy(&task_num_);//销毁信号量

  for (int i = 0; i < work_threads_.size(); ++i)
  {
    while (pthread_kill(work_threads_[i]->work_thread_id(), 0) == 0)
    {
      work_threads_[i]->get_condition().notify();//通知激活该线程
      select(0, NULL, NULL, NULL, &delay);
    }
  }
  INFO("Thread pool is closed successfully.\n");
}

} // namespace http_server