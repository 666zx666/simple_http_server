/**
 * @file Socket.h
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef SOCKET_H_
#define SOCKET_H_

#include <boost/noncopyable.hpp>

namespace http_server
{

/*
*@brief 套接字类
*/
class Socket : public boost::noncopyable
{
public:
  explicit Socket(int listen_port);

  Socket() = delete;//禁止编译器使用该方法

  int fd() const { return sockfd_; }

  void set_reuseaddr();

  void bind();

  void listen();

  int accept();

  void close();

private:
  int listen_port_;
  int sockfd_;
};

}  // namespace http_server

#endif  // SOCKET_H_