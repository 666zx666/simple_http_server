/**
 * @file Socket.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "Socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <iostream>

#define LOGGER_WARN
#include <logger.h>

namespace http_server
{
/**
 * @brief Socket构造函数，套接字初始化.
 * 
 */
Socket::Socket(int listen_port)
  : listen_port_(listen_port),
    sockfd_(socket(AF_INET, SOCK_STREAM, 0))
{
}

/**
 * @brief 设置套接字本地地址复用.
 * 
 */
void Socket::set_reuseaddr()
{
  int reuse = 1;
  assert(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == 0);
}

/**
 * @brief 设置服务器地址并绑定地址端口.
 * 
 */
void Socket::bind()
{
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));//清零
  server_addr.sin_family = AF_INET;//协议族
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//任意本地地址
  server_addr.sin_port = htons(listen_port_);//服务器端口
  int err=::bind(sockfd_, (sockaddr*)&server_addr, sizeof(server_addr));//全局作用域中的绑定函数
  if(err<0){
    WARN("bind error！\n");
    return;
  }
}

/**
 * @brief 设置侦听队列.
 * 
 */
void Socket::listen()
{
  int err=::listen(sockfd_, 10000);
  if(err<0){
    WARN("listen error！\n");
    return;
  }
}

/**
 * @brief 接受客户端连接
 * 
 * @return int Client fd;
 */
int Socket::accept()
{
  return ::accept(sockfd_, NULL, 0);
}

/**
 * @brief 关闭套接字.
 * 
 */
void Socket::close()
{
  ::close(sockfd_);
}


} // namespace http_server