/**
 * @file my_thread.h
 * @author zX
 * @brief Thread Class Which Imitates the std::thread
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef MY_THREAD_H_
#define MY_THREAD_H_

#include <boost/function.hpp>
#include <boost/atomic.hpp>
#include <my_thread.h>

namespace my_thread
{
/**
 * @brief 线程回调函数
 * @param void* arg
 */
template <typename CLASSTYPE>
void* thread_func(void* arg)
{
  CLASSTYPE *thisClass = (CLASSTYPE*)arg;
  thisClass->func_();
  CLASSTYPE::thread_num--;
  return NULL;
}

/**
 * @brief 线程类 （模仿std::thread）
 */
class Thread
{
public:
  typedef std::function<void ()> ThreadFunc;//std::function 是一个可调用对象void()的包装器，取代函数指针的作用

  template<typename _func, typename... _Args>
  explicit Thread(_func&& _threadFun, _Args&&... _args)//参数为右值引用 
    : started_(false), 
      joined_(false), 
      detached_(false)
  {
    func_ = std::bind(std::forward<_func>(_threadFun), std::forward<_Args>(_args)...);//将所有参数_args与函数_threadFun绑定，std::forward保证参数按照实际左（右）值传入
    thread_num++;

  }

  void start();

  void join();

  /*
  *@brief 获取线程ID
  *@return pthread_t类型的线程ID
  * */
  pthread_t thread_id()
  {
    return id_;
  }

  void detach();

  ~Thread(){}

  ThreadFunc func_;

  static boost::atomic_int thread_num;
private:
  pthread_t id_;//线程ID
  bool started_;//线程已启动标志位
  bool joined_;//线程已结束标志位
  bool detached_;//线程已分离标志位

  Thread(const Thread&);
  Thread& operator=(const Thread&);
};



} // MyThread

#endif