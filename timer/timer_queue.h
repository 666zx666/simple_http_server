/**
 * @file timer_queue.h
 * @author zX
 * @brief timer queue for thread safety
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef TIMER_QUEUE_H_
#define TIMER_QUEUE_H_

#include <list>
#include <timer_tick.h>
#include <my_mutex.h>

namespace timer_tick
{

/*
*@brief 定时器队列类（队列中计时器按超时时间从小到大排列）
*/
class TimerQueue
{
public:
  TimerQueue()
  {
  }

  ~TimerQueue(){}

/**
 * @brief 添加新的定时器到定时器链表队列，并设置其迭代器.
 * 
 * @param Timer* new_timer 
 */
  void add_timer(Timer* new_timer)
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    if(timer_queue_.empty())
    {
      timer_queue_.push_back(new_timer);
      new_timer->set_iter(timer_queue_.begin());
      return;
    }

    for(auto iter = timer_queue_.begin(); iter != timer_queue_.end(); ++iter)
    {
      //队列按超时时间从小到大排列
      if(new_timer->overtime() < (*iter)->overtime())
      {
        auto new_iter = timer_queue_.insert(iter, new_timer);
        new_timer->set_iter(new_iter);
        return;
      }
    }
    auto new_iter = timer_queue_.insert(timer_queue_.end(), new_timer);
    new_timer->set_iter(new_iter);
  }

/**
 * @brief 删除指向队列中迭代器指向的定时器。如果定时器不是在队列中,程序会崩溃。
 * 
 * @param Timer* del_timer 
 */
  void del_timer(Timer* del_timer)
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    timer_queue_.erase(del_timer->iter());
  }

  /**
 * @brief 获取定时器队列队首元素
 * 
 * @return Timer*  timer_queue_.begin()
 */
  Timer* top()
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    if(timer_queue_.empty())
      return nullptr;
    else
      return *(timer_queue_.begin());
  }

  /**
 * @brief 删除定时器队列的队首元素
 * 
 * @param Timer* del_timer 
 */
  void pop()
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    if(!timer_queue_.empty())
    {
      timer_queue_.erase(timer_queue_.begin());
    }
  }

   /**
 * @brief 判断定时器队列是否为空
 * 
 * @return 为空返回true，否则返回false 
 */
  bool empty()
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    return timer_queue_.empty();
  }
 
 /**
 * @brief 获取定时器队列大小
 * 
 * @return 定时器队列大小int 
 */
  int size()
  {
    my_mutex::MutexLockGuard mlg(mutex_);
    return timer_queue_.size();
  }

private:
  std::list<Timer*> timer_queue_;//计时器链表队列，队列中计时器按超时时间从小到大排列
  my_mutex::MutexLock mutex_;
};


} // namespace timer_tick


#endif // TIMER_QUEUE_H_