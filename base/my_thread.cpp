/**
 * @file my_thread.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include <my_thread.h>

namespace my_thread
{

boost::atomic_int Thread::thread_num(0); //记录创建的线程的数量。

/**
 * @brief 线程创建函数。必须在线程类建立后调用，否则新的线程不会被创建。
 */
void Thread::start()
{
  if (!started_)
  {
    pthread_create(&id_, NULL, thread_func<Thread>, this);//id为线程ID，thread_func<Thread>为线程处理回调函数
    started_ = true;
  }
  else
    return;
}

/**
 * @brief 线程结束函数。以阻塞的方式等待指定的joinable状态的线程调用此函数后结束线程。当函数返回时，被等待线程的资源被收回。
 */
void Thread::join()
{
  if (started_ && !joined_)
    pthread_join(id_, NULL);
  return;
}

/**
 * @brief 线程分离函数。将状态改为unjoinable状态，这些资源在线程函数退出时或pthread_exit时自动会被释放。
 */
void Thread::detach()
{
  if (started_ && !detached_ && !joined_)
    pthread_detach(id_);
  else
    return;
}

} // namespace my_thread
