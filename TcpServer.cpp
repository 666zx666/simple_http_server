/**
 * @file TcpServer.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "TcpServer.h"
#include "Socket.h"
#include "thread_pool.h"
#include <fcntl.h>//unix标准中通用的头文件

namespace http_server
{

TcpServer::TcpServer(ThreadPool* thread_pool, int listen_port)
  : thread_pool_(thread_pool),
    socket_(new Socket(listen_port))
{
  socket_->set_reuseaddr();
  socket_->bind();
  socket_->listen();
}

/**
 * @brief 添加新的工作任务进线程池的工作队列
 * 
 * @param std::function<void ()> new_job 
 */
status TcpServer::add_task_to_pool(std::function<void ()> new_job)
{
  thread_pool_->add_task_to_pool(new_job);
}

/**
 * @brief 设置套接字fd为非阻塞
 * 
 * @param fd 
 */
void TcpServer::setNoBlock(int fd)
{
  int old_option = fcntl(fd, F_GETFL);//获取文件的状态标志属性
  int new_option = old_option | O_NONBLOCK;//向旧属性值中增加非阻塞选项O_NONBLOCK
  fcntl(fd, F_SETFL, new_option);//设置文件新的状态标志属性
}

}