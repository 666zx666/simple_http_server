/**
 * @file work_thread.h
 * @author zX
 * @brief work class and work thread class
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef WORK_THREAD_H_
#define WORK_THREAD_H_

#include <boost/noncopyable.hpp>//noncopyable类阻止派生类拷贝构造和赋值构造。
#include <memory>//使用智能指针

#include <my_thread.h>
#include <my_mutex.h>
#include <my_condition.h>
#include "work_queue.h"

#include <iostream>

using namespace my_mutex;
using namespace my_condition;

namespace http_server
{

enum thread_state{BOOTING, READY, IDLE, BUSY, QUIT};//线程状态

namespace work_thread
{

/**
 *@brief 线程中的工作Work类，包含线程工作的执行函数
 * 
 */
class Work
{
public:
  typedef std::function<void ()> work_func;
  typedef std::shared_ptr<Work> WorkPtr;

public:
  Work(work_func work) : work_(work)
  { 
  }

  Work(){}

  /*@brief 执行工作*/
  void execute_work();

  /*
  *@brief 创建工作
  *@return 返回指向创建的新的工作的指针WorkPtr
  */
  static WorkPtr create_work(work_func work)
  {
    return WorkPtr(new Work(work));
  }

  ~Work(){}


private:
  work_func work_;

};

/**
 * @brief 工作线程类
 * 
 */
class WorkThread : public boost::noncopyable
{
public:
  typedef std::shared_ptr<WorkThread> WorkThreadPtr;
  typedef std::function<void (void*)> ThreadFunc;
  thread_state state_;

public:
  WorkThread(ThreadFunc func) 
  : thread_func_(func),
    mutex_(),
    pcond_(mutex_),
    thread_(new my_thread::Thread(thread_func_, this))
  {
  }

  ~WorkThread() {}

  /*@brief 工作线程初始化*/
  void start()
  {
    thread_->start();
    thread_id_ = thread_->thread_id();
  }

  /*
  *@brief 获取工作线程ID
  *@return 工作线程ID pthread_t thread_id_
  * */
  pthread_t work_thread_id()
  {
    return thread_id_;
  }

  /*
  *@brief 获取互斥锁
  *@return  互斥锁MutexLock& mutex_
  */
  MutexLock& get_mutex()
  {
    return mutex_;
  }

  /*
  *@brief 获取工作条件
  *@return 工作条件Condition& pcond_
  */
  Condition& get_condition()
  {
    return pcond_;
  }

  inline void add_work(Work::WorkPtr new_work);

  inline Work::WorkPtr pop_work();

  inline bool work_empty();

private:
  ThreadFunc thread_func_;
  MutexLock mutex_;
  Condition pcond_;
  
  std::shared_ptr<my_thread::Thread> thread_; 
  pthread_t thread_id_;

  WorkQueue<Work::WorkPtr> work_queue_;//工作队列

};

/*
*@brief 添加新的工作至工作线程的工作队列
*@param 新的工作内容Work::WorkPtr new_work
*/
inline void WorkThread::add_work(Work::WorkPtr new_work)
{
  work_queue_.push_work(new_work);
}

/*
*@brief 删除工作线程的工作队列队首的已有工作
*@return 返回指向工作队列队首的指针Work::WorkPtr
*/
inline Work::WorkPtr WorkThread::pop_work()
{
  Work::WorkPtr tmp = work_queue_.pop_work();
  return tmp;
}

/*
*@brief 判断工作线程的工作队列是否为空
*@return 为空返回true；否则，返回false
*/
inline bool WorkThread::work_empty()
{
  return work_queue_.empty();
}

} // namespace work_thread

} // namespace http_server
 
#endif // WORK_THREAD_H_