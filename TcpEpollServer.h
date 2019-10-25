/**
 * @file TcpEpollServer.h
 * @author zX
 * @brief Use epoll to handle client request.
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef TCPEPOLLSERVER_H_
#define TCPEPOLLSERVER_H_
#include "TcpServer.h"
#include "parameters.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <map>
#include <string>
#include <queue>
#include <timer_tick.h>
#include <timer_queue.h>

namespace http_server
{

class ThreadPool;

class TcpEpollServer : public TcpServer
{
public:
  TcpEpollServer(ThreadPool* pool, parameters::Parameters* parameters);

  virtual void handle_request() override;

  virtual void add_event(int fd, int event_type) override;

  virtual void del_event(int fd, int event_type) override;

  static void sig_int_handle(int sig);

  virtual void client_service(int client_fd) override;

  virtual ~TcpEpollServer();

  void close_client(int fd);

  void file_mmap();

  int get_line(int sock, char *buf, int size);
  void unimplemented(int client);
  void not_found(int client);
  void execute_cgi(int client, const char *path, const char *method, const char *query_string);
  void doGetMethod(int client_fd, char *url, char *version);
  void file_serve(int client_fd, char * filename);
  void headers(int client);
  void send_file(int client, std::string filename);
  void client_overtime_cb(timer_tick::Timer* overtime_timer);

  static const int MAXEVENTS = 255;
  static const int METHOD_LEN = 255;
  static const int URL_LEN = 255;
  static const int VERSION_LEN = 50;
  static const int CLIENT_LIFE_TIME = 5;
  static const int MAX_FD = 10000;

private:
  int epoll_fd_;
  char *document_root_;
  char *default_file_;
  parameters::Parameters *http_parameters_;

  std::map<std::string, char*> http_file_;  // http文件路径和相应的mmap addr.
  std::vector<int> file_fd_lists_; // http文件描述符数组.

  static int efd_; // 事件文件描述符(event_fd）

  timer_tick::TimerQueue client_timers_queue_;  // 客户端定时器队列 client timer queue
  timer_tick::Timer* client_fd_array_[MAX_FD];  // 客户端套接字对应的定时器 client fd and its timer

};



} // namespace http_server


#endif // TCPEPOLLSERVER_H_