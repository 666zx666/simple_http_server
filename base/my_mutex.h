/**
 * @file my_mutex.h
 * @author zX
 * @brief RAII Mutex
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef MY_MUTEX_H_
#define MY_MUTEX_H_
#include <boost/noncopyable.hpp>//（继承boost::noncopyable，除了子类自己定义拷贝构造函数或者复制构造函数，外部的调用者不能够通过拷贝构造函数或者复制构造函数创建一个新的子类对象的。）
#include <pthread.h>

namespace my_mutex{

/**
 * @brief 互斥锁类
 */
class MutexLock : public boost::noncopyable  
{
public:
  MutexLock()
  {
    pthread_mutex_init(&mutex_, NULL);
  }

  ~MutexLock()
  {
    pthread_mutex_destroy(&mutex_);
  }

  void Lock()
  {
    pthread_mutex_lock(&mutex_);
  }

  void Unlock()
  {
    pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t* getPthreadMutex(){ return &mutex_; }

private:
  pthread_mutex_t mutex_;
};

/**
 * @brief 调用互斥锁类
 */
class MutexLockGuard : public boost::noncopyable
{
public:
  explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex)
  {
    mutex_.Lock();
  }

  ~MutexLockGuard()
  {
    mutex_.Unlock();
  }

private:
  MutexLock& mutex_;
};

}  //MyMutex

#endif
