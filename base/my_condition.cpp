/**
 * @file my_condition.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include <my_condition.h>
#include <errno.h>
#include <time.h>

using namespace my_mutex;

namespace my_condition
{
bool Condition::waitForSeconds(double seconds)
{
  struct timespec abstime;
  clock_gettime(CLOCK_REALTIME, &abstime);

  const int64_t kNanoSecondsPerSecond = 1000000000;
  int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

  abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
  abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

  MutexLockGuard lg(mutex_);
  return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);//条件等待超时解除阻塞，超时时间为abstime，超时返回错误码ETIMEDOUT
}


}