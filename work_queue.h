/**
 * @file work_queue.h
 * @author zX
 * @brief Work Queue For Thread Safety
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef WORKQUEUE_H_
#define WORKQUEUE_H_

#include <my_mutex.h>
using namespace my_mutex;

/*@brief 工作节点结构*/
template <class T>
struct work_node
{
  work_node *next;
  T work;
};

/*@brief 工作队列类*/
template <class T>
class WorkQueue
{
private:
  work_node<T> *head_node;//头节点
  work_node<T> *cur;//指向当前节点
  MutexLock mutex;
  boost::atomic_int queue_size_; // 记录队列的大小。可以更换为int。

public:
  WorkQueue() : queue_size_(0)
  {
    head_node = new work_node<T>;
    head_node->next = nullptr;
    cur = head_node;
  }

  /*
  *@brief 添加新的工作节点
  *@param 模板类T work
  */
  void push_work(T work)
  {
    MutexLockGuard mlg(mutex);
    work_node<T> *new_node = new work_node<T>;
    new_node->work = work;
    new_node->next = nullptr;
    cur->next = new_node;
    cur = new_node;
    queue_size_++;
  }

  /*
  *@brief 判断工作队列是否为空
  *@return 为空返回true；否则，返回fasle
  */
  bool empty()
  {
    MutexLockGuard mlg(mutex);
    if(cur == head_node)
      return true;
    else
      return false;
    
  }

  /*void pop_work()
  {
    MutexLockGuard mlg(mutex);
    if(cur == head_node)
      return;
    work_node<T> *temp = head_node->next;
    if(cur == temp)
      cur = head_node;
    head_node->next = temp->next;
    delete temp;
  }*/


  /*
  *@brief 删除并返回工作队列的队首节点
  *@return 返回模板类T work_to_pop
  */
  T pop_work()
  {
    MutexLockGuard mlg(mutex);
    if(cur == head_node)
      return nullptr;
    work_node<T> *temp = head_node->next;
    if(cur == temp)
      cur = head_node;
    head_node->next = temp->next;
    T work_to_pop = temp->work;
    delete temp;
    queue_size_--;
    return work_to_pop;
  }

  /*
  *@brief 获取队首节点内容
  *@return 返回模板类T
  */
  T top()
  {
    MutexLockGuard mlg(mutex);
    return head_node->next->work;
  }

  /*
  *@brief 获取队列大小
  *@return 队列大小int size
  */
  int size()
  {
    /*MutexLockGuard mlg(mutex);
    work_node<T> *pNode = head_node->next;
    int size = 0;
    while(pNode!=nullptr)
    {
      size++;
      pNode = pNode->next;
    }
    return size;*/
    int size = static_cast<int>(queue_size_);
    return size;
  }

  ~WorkQueue()
  {
    while(head_node->next!=nullptr)
    {
      work_node<T> *delNode = head_node->next;
      head_node->next = delNode->next;
      delete delNode;
    }
    delete head_node;
  }
};

#endif