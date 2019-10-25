/**
 * @file timer_tick.h
 * @author zX
 * @brief Timer class. The timer contain overtime, callback function, iterator in queue and fd.
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef TIMER_TICK_H_
#define TIMER_TICK_H_

#include <functional>
#include <time.h>
#include <list>

namespace timer_tick
{

/*
*@brief 定时器类
*/
class Timer
{
public:
  typedef std::function<void (Timer*)> callback_func_;
  Timer(int fd, callback_func_ func, time_t overtime = 0) 
    : fd_(fd), 
      overtime_callback_(func), 
      overtime_(overtime)
  {
  }

  ~Timer(){}

  /*
  *@brief 设置回调函数
  *@param std::function<void (Timer*)> func
  */
  void set_callback_func(callback_func_ func)
  {
    overtime_callback_ = func;
  }

  /*
  *@brief 超时回调函数
  *@param Timer* timer
  */
  void overtime_callback(Timer* timer)
  {
    if(overtime_callback_)
      overtime_callback_(timer);
  }

  /*
  *@brief 设置超时时间
  *@param time_t overtime
  */
  void set_overtime(time_t overtime)
  {
    overtime_ = overtime;
  }

  /*
  *@brief 获取超时时间
  *@return  超时时间 time_t overtime_
  */
  time_t overtime()
  {
    return overtime_;
  }

  bool operator<(const Timer& b)
  {
    if(overtime_ < b.overtime_)
      return true;
    else
      return false; 
  }

  /*
  *@brief 设置迭代器
  *@param std::list<Timer*>::iterator iter
  */
  void set_iter(std::list<Timer*>::iterator iter)
  {
    iter_ = iter;
  }

  /*
  *@brief 获取链表迭代器
  *@return  std::list<Timer*>::iterator
  */
  std::list<Timer*>::iterator iter()
  {
    return iter_;
  }

  /*
  *@brief 获取套接字
  *@return  套接字 int fd_
  */
  int fd()
  {
    return fd_;
  }

private:
  int fd_;
  callback_func_ overtime_callback_;//超时回调函数对象
  time_t overtime_;//超时时间
  std::list<Timer*>::iterator iter_;//list<Timer*> 链表迭代器
};


} // namesapce timer_tick



#endif  // TIMER_TICK_H_