/**
 * @file my_condition.h
 * @author zX
 * @brief RAII Condition
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef MY_CONDITION_H_
#define MY_CONDITION_H_

#include <my_mutex.h>
#include <boost/noncopyable.hpp>

using namespace my_mutex;

namespace my_condition
{
/*@brief 线程同步条件变量类*/
class Condition : public boost::noncopyable
{
public:
  /*
   * @brief 构造函数，初始化一个条件变量
   * @param 互斥锁MutexLock& mutex
   */
  explicit Condition(MutexLock& mutex) : mutex_(mutex)
  {
    pthread_cond_init(&pcond_, NULL);
  }

  /*@brief 析构函数，销毁一个条件变量 */
  ~Condition()
  {
    pthread_cond_destroy(&pcond_);
  }

  /*@brief 通知激活某一个等待在条件变量上的线程 */
  void notify()
  {
    MutexLockGuard lg(mutex_);
    pthread_cond_signal(&pcond_);
  }

  /*@brief 广播激活所有等待在条件变量上的线程 */
  void notifyAll()
  {
    MutexLockGuard lg(mutex_);
    pthread_cond_broadcast(&pcond_);
  }

  /*@brief 使当前线程阻塞在pcond_指向的条件变量上*/
  void wait()
  {
    MutexLockGuard lg(mutex_);
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
  }

  /* 
   * @brief 判断等待超时函数。
   * @param 当前线程阻塞条件变量等待时间参数double seconds
   * @return 若超时返回1，否则返回0
   */
  bool waitForSeconds(double seconds);

private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;

};

} // namespace my_condition


#endif  //MY_CONDITION_H_