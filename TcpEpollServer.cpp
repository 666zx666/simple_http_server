/**
 * @file TcpEpollServer.cpp
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "TcpEpollServer.h"
#include "parameters.h"
#include <algorithm>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>

//#define LOGGER_DEBUG
#define LOGGER_WARN
#include <logger.h>

#include "Socket.h"

namespace http_server
{

static const int MAX_MMAP_LENGTH = 10000;


TcpEpollServer::TcpEpollServer(ThreadPool *pool, parameters::Parameters *parameters)
  : TcpServer(pool, parameters->getListenPort()),
    http_parameters_(parameters)
{
  document_root_ = parameters->getDocumentRoot();
  default_file_ = parameters->getDefaultFile();
  file_mmap();
 // efd_ = eventfd(0, 0);
}

int TcpEpollServer::efd_ = eventfd(0, 0);  //事件文件描述符（event fd)。初始化其内部计数器count为0。flags 设置为 0，
                                           //用于sigint退出handle_request循环。用户空间的应用程序可以用这个 eventfd 来实现事件的等待或通知机制，也可以用于内核通知新的事件到用户空间应用程序。

/**
 * @brief Map the file to memory.文件映射到内存中。
 * 
 */
void TcpEpollServer::file_mmap()
{
  std::vector<std::string> file_lists = http_parameters_->getHttpFileLists();// 值为"doc/index.html"
  
  file_fd_lists_.resize(file_lists.size());

  for(int i = 0; i < file_lists.size(); ++i)
  {
    file_fd_lists_[i] = open(file_lists[i].c_str(), O_RDONLY);//打开file_lists[i].c_str()指定的文件名，为只读模式。返回一个文件描述符，出错时返回-1
    if(file_fd_lists_[i] == -1)
    {
      WARN("open file %s failed!\n", file_lists[i].c_str());
      continue;
    }
    char* file_ptr = (char *)mmap(NULL, MAX_MMAP_LENGTH, PROT_READ, MAP_SHARED, file_fd_lists_[i], 0);//将一个文件或者其它对象映射进内存，成功时返回被映射区的指针
    if((char*)-1 == file_ptr)
    {
      WARN("mmap failure\n");
    }
    else
    {
      http_file_[file_lists[i]] = file_ptr;
    }
  }
}

TcpEpollServer::~TcpEpollServer()
{
  for(int i = 0; i < file_fd_lists_.size(); ++i)
  {
    if(file_fd_lists_[i] != -1)
      close(file_fd_lists_[i]);
  }

  for(auto iter = http_file_.begin(); iter != http_file_.end(); ++iter)
  {
    munmap(iter->second, MAX_MMAP_LENGTH);//解除一个映射关系，iter->second为调用时返回的地址
  }
}


/**
 * @brief SIGINT 信号回调函数. 事件文件描述符 efd 被用来通知主循环退出
 * 
 * @param sig 
 */
void TcpEpollServer::sig_int_handle(int sig)
{
  uint64_t u = 1;
  ssize_t rc;
  rc = write(efd_, &u, sizeof(uint64_t));//将u的值增加到efd的计数器count中
  if(rc != sizeof(uint64_t))
    WARN("sig int eventfd write error");
}

/**
 * @brief 注册添加新的fd到 epoll fd
 * 
 * @param fd 
 * @param event_type 
 */
void TcpEpollServer::add_event(int fd, int event_type)
{
  epoll_event e;
  e.data.fd = fd;
  e.events = event_type;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &e);//宏EPOLL_CTL_DE：注册新的fd到epfd中
}

/**
 * @brief 从epoll fd中删除fd
 * 
 * @param fd 
 * @param event_type 
 */
void TcpEpollServer::del_event(int fd, int event_type)
{
  epoll_event e;
  e.data.fd = fd;
  e.events = event_type;
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &e);//宏EPOLL_CTL_DEL：从epfd中删除一个fd；
}

/**
 * @brief 关闭客户端的 fd 并从客户端定时器队列中删除其计时器timer 
 * 
 * @param fd 
 */
void TcpEpollServer::close_client(int fd)
{
  client_timers_queue_.del_timer(client_fd_array_[fd]);
  delete client_fd_array_[fd];
  client_fd_array_[fd] = nullptr;
  close(fd);
}

/**
 * @brief 从客户端fd中读取数据。从客户端获取服务请求并应答。
 * 
 * @param client_fd 
 */
void TcpEpollServer::client_service(int client_fd)
{
  DEBUG("handling client request... client fd: %d\n", client_fd);

  char rcv_buffer[BUFSIZ];
  char method[METHOD_LEN];
  char url[URL_LEN];
  char version[VERSION_LEN];

  char if_close;  
  int ret = recv(client_fd, &if_close, 1, MSG_PEEK); //如果客户端以正常方式关闭连接，返回值为0。MSG_PEEK用于查看可读数据，在函数执行后内核不会丢弃这些数据。
  if(ret == 0)
  {
    close_client(client_fd);
    DEBUG("client close itself\n");
    return;
  }

  int n;
  n = get_line(client_fd, rcv_buffer, BUFSIZ);
  DEBUG("%s\n", rcv_buffer);  //GET /index.html HTTP/1.0

  int i = 0, j = 0;
  while (rcv_buffer[j] != ' ' && i <= METHOD_LEN - 1 && j <= n) //get the method
  {
    method[i] = rcv_buffer[j];
    ++i;
    ++j;
  }
  method[i] = '\0';
  DEBUG("method: %s\n", method);

  i = 0;
	while( rcv_buffer[j] ==' ' && j <= n)
		j++;
	while( rcv_buffer[j] != ' ' && j <= n && i < URL_LEN-1)//get the URL
	{
		url[i] = rcv_buffer[j];
		i++;
		j++;
	}
	url[i] = '\0';
	DEBUG("url: %s\n", url);

  i = 0;
	while(rcv_buffer[j] == ' ' && j <= n)
		j++;
	while(rcv_buffer[j] != '\n' && i < VERSION_LEN-1)
	{
		version[i] = rcv_buffer[j];
		i++;
		j++;
	}
	version[i] = '\0';
	DEBUG("version: %s\n", version);

  if(strcasecmp(method, "GET") && strcasecmp(method, "POST"))//strcasecmp判断字符串是否相等(忽略大小写)
	{
		//can not under stand the request
		unimplemented(client_fd);
		close_client(client_fd);
		return;
	}

  if(strcasecmp(method, "GET") == 0)
	{
		//call GET method function
		doGetMethod(client_fd, url, version);
	}
	else if(strcasecmp(method, "POST") == 0)
	{
		//call POST function
    close_client(client_fd);
	}

}

/**
 * @brief 采用epoll方法处理请求循环 
 * 
 */
void TcpEpollServer::handle_request()
{
  signal(SIGPIPE, SIG_IGN);  //把SIGPIPE设为SIG_IGN，SIGPIPE交给了系统处理，客户端不退出
  signal(SIGINT, sig_int_handle); 

  if(socket_->fd() == -1)
  {
    WARN("Create listen fd failed!\n");
    return;
  }

  assert(efd_ != -1);

  //使用 epoll 监听定时器到期事件
  epoll_fd_ = epoll_create(255);//生成一个epoll专用的文件描述符,用来存放所关注的fd，监听最大数目为255
  assert(epoll_fd_ != -1);

  int time_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);  //创建计时器timefd,类型为系统范围内的时钟,用来周期检查定时器队列
  assert(time_fd != -1);
  struct itimerspec new_value;
  struct timespec now;
  uint64_t exp;
  ssize_t s;
  assert(clock_gettime(CLOCK_REALTIME, &now) != -1);
  new_value.it_value.tv_sec = 2;//it_value表示定时器第一次超时时间
  new_value.it_value.tv_nsec = now.tv_nsec;
  new_value.it_interval.tv_sec = 2;//it_interval表示之后的超时时间即每隔多长时间超时
  new_value.it_interval.tv_nsec = 0;
  assert(timerfd_settime(time_fd, 0, &new_value, NULL) != -1);  // 启动定时器，间隔周期 2s
  bool timer_tick = false;

  add_event(socket_->fd(), EPOLLIN);//EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）
  
  add_event(efd_, EPOLLIN);
   
  add_event(time_fd, EPOLLIN);

  int overtime_ms = -1;//超时时间，ms级
  bool run = true;
  epoll_event events[MAXEVENTS];//epoll 事件数组
  while(run == true)
  {
    int ret = epoll_wait(epoll_fd_, events, MAXEVENTS, overtime_ms); //等待注册在epoll_fd_上的事件的发生,如果发生则将发生的sokct fd和事件类型放入到events数组中。并将注册在epfd上的socket fd的事件类型给清空（fd并未清空）。
                                                                     //返回需要处理的事件数目，如返回0表示已超时。
    if(ret <= 0 && errno != EINTR)//如果返回不为正数，且错误类型不为接收到中断信号
    {
      WARN("epoll wait failed!\n");
      break;
    }

    for(int i = 0; i < ret; ++i)
    {
      if(events[i].data.fd == socket_->fd())   // 若得到的是服务器socket fd ,则待处理事件为一个客户端
      {
        int client_fd = socket_->accept();
        if(client_fd == -1)
        {
          continue;
        }
        setNoBlock(client_fd);
        
        add_event(client_fd, EPOLLIN); //将客户端client_fd注册加入epoll fd
        
        timer_tick::Timer *new_timer = new timer_tick::Timer(
          client_fd, std::bind(&TcpEpollServer::client_overtime_cb, this, std::placeholders::_1), time(NULL) + CLIENT_LIFE_TIME);    //  创建client fd的定时器
        assert(client_fd < MAX_FD);
        client_fd_array_[client_fd] = new_timer;   // 记录fd和计时器
        client_timers_queue_.add_timer(new_timer);  

        DEBUG("accept a new client[%d]\n", client_fd);
      }
      else if(events[i].data.fd == time_fd)  // 若得到的是 timer fd ，则timer_tick设置为true，检查定时器队列
      {
        s = read(time_fd, &exp, sizeof(uint64_t));
        assert(s == sizeof(uint64_t));
        timer_tick = true;
        DEBUG("timer tick!!!\n");
      }
      else if((events[i].data.fd == efd_ ) && (events[i].events & EPOLLIN))  // 若得到的是event fd (即SIGINT信号），则退出循环.
      {
        INFO("Got a sigint signal. Exiting...\n");
        run = false;
        break;
      }
      else if(events[i].events & EPOLLIN) // 若为客户端发送请求
      {
        DEBUG("receive a request from client[%d]\n", events[i].data.fd);
        status r = 
          add_task_to_pool(std::bind(
            &TcpEpollServer::client_service, this, static_cast<int>(events[i].data.fd)));  // 添加工作到线程池
        /*if(r == FAILED)
        {
          continue;  // How to handle overflowed task?
        }*/
        del_event(events[i].data.fd, EPOLLIN);
      }
      else if(events[i].events & EPOLLRDHUP) // 客户端关闭了fd. EPOLLRDHUP表示对端断开连接
      {
        DEBUG("EPOLLRDHUP!\n");
        close_client(events[i].data.fd);
        del_event(events[i].data.fd, EPOLLIN);
      }
      else
      {
        WARN("unexpected things happened! \n");
      }
    }

    if(timer_tick)   // timer ticking. 删除剩余的客户链接。
    {
      time_t current_time = time(NULL);//获取系统时间，单位为秒;
      while(!client_timers_queue_.empty())
      {
        timer_tick::Timer* top_timer = client_timers_queue_.top();
        if(top_timer->overtime() < current_time)
        {
          top_timer->overtime_callback(top_timer);   //如果超时,调用回调函数来处理。
        }
        else
        {
          break;
        }   
      }
      DEBUG("client queue size: %d\n", client_timers_queue_.size());
    }
  }
  socket_->close();
}

/**
 * @brief 函数从一个套接字fd得到一行字符串文本，并存于buf指向的空间中。
 *        
 * @param sock 
 * @param buf 
 * @param size 
 * @return int 
 */
int TcpEpollServer::get_line(int sock, char *buf, int size)
{
  int i = 0;
	char c = '\0';
	int n;
  
  //读取直到换行,并且不管是'\r\n'结尾还是'\n'结尾，都统一成'\n'
	while ((i < size - 1) && (c != '\n'))
	{
	n = recv(sock, &c, 1, 0);//读取一个字符
	if(n > 0)//若读到了这个字符
	{
		if(c == '\r')//若是'\r'就继续读，因为可能是'\r''\n'换行的
		{
			n = recv(sock, &c, 1, MSG_PEEK);//MSG_PEEK参数类似于预读取，本次读取的值仍在缓冲区仍可重复被读
			if((n > 0) && (c == '\n'))//看看预读的数是否是'\n'
			 recv(sock, &c, 1, 0);//如果是的话，就把'\n'彻底的从缓冲区中拿出来
			else
			 c = '\n';//如果下个数不是'\n'，就把'\r'也替换成'\n'
	  }
	   buf[i] = c;//将字符存再buf中
	   i++;
	}
	else
		c = '\n';
	}
	buf[i] = '\0';//最后以'\0'结尾
 
 return i;
}

/**
 * @brief 回复客户端服务器无法接受请求
 * 
 * @param client 
 */
void TcpEpollServer::unimplemented(int client)
{
  char buf[1024];
  sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);//MSG_NOSIGNAL，禁止send()函数向系统发送异常消息
  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "</TITLE></HEAD>\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

/**
 * @brief 响应 GET 请求
 * 
 * @param client_fd 
 * @param url file path
 * @param version http version
 */
void TcpEpollServer::doGetMethod(int client_fd, char *url, char *version)
{
  char path[BUFSIZ];
  struct stat st;//stat结构体是用来描述一个linux系统文件系统中的文件属性的结构。
  char *query_string = NULL;
  query_string = url;
  while( *query_string != '?' && *query_string != '\0')
		query_string++;
	if(*query_string == '?')
	{
		*query_string = '\0';
		query_string++;
	}
  sprintf(path, "%s%s", document_root_, url);		
  if(path[strlen(path)-1] == '/')//如果最后为'/'，将default_file_添加到path之后
		strcat(path, default_file_);
  
  DEBUG("Finding the file: %s\n", path);

  if(stat(path, &st) == -1)//stat()通过文件名path获取文件信息，并保存在st所指的结构体stat中,执行成功则返回0，失败返回-1
  {
    DEBUG("can not find the file: %s\n", path);
    close_client(client_fd);
    return;
  }
  else
  {
    if(S_ISDIR(st.st_mode))//是否为目录
    {
      strcat(path, "/");
      strcat(path, default_file_);
    }

    if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode &S_IXOTH)//文件所有者具可执行权限、用户组具可读取权限、其他用户具可执行权限
		{
			//CGI server
			execute_cgi(client_fd, path, "GET", query_string); 				
			close_client(client_fd);
		}
    else
    {
      file_serve(client_fd, path);
      close_client(client_fd);
    }
  }
  
}

/**
 * @brief 找不到请求，回应 404 给客户端
 * 
 * @param client 
 */
void TcpEpollServer::not_found(int client)
{
  char buf[1024];
  DEBUG("send the 404 infomation!\n");
  sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "your request because the resource specified\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "is unavailable or nonexistent.\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

void TcpEpollServer::execute_cgi(int client, const char *path, const char *method, const char *query_string)
{
}

/**
 * @brief 发送文件到客户端。
 * 
 * @param client_fd 
 * @param filename 
 */
void TcpEpollServer::file_serve(int client_fd, char * filename)
{
  char buffer[BUFSIZ];
  int n = 1;
  buffer[0] = 'A';
  buffer[1] = '\0';
  while(n > 0 && strcmp(buffer, "\n"))//get and discard headers
  {
    get_line(client_fd, buffer, sizeof(buffer));
    DEBUG("%s\n", buffer);
  }

  std::string file = filename;
  if(http_file_.find(filename) == http_file_.end())
  {
    WARN("can not open the file: %s\n", filename);
    not_found(client_fd);
    close_client(client_fd);
    return;
  }
  else
  {
    DEBUG("Now send the file\n");
    headers(client_fd);
    send_file(client_fd, file);
  }
}

/**
 * @brief 发送 http header(200 OK) 给客户端.
 * 
 * @param client 
 */
void TcpEpollServer::headers(int client)
{
  char buf[1024];

  strcpy(buf, "HTTP/1.0 200 OK\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  strcpy(buf, "connection:keep-alive\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  strcpy(buf, SERVER_STRING);
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
  strcpy(buf, "\r\n");
  send(client, buf, strlen(buf), MSG_NOSIGNAL);
}

/**
 * @brief 从mmap发送文件的内容
 * 
 * @param client 
 * @param filename 
 */
void TcpEpollServer::send_file(int client, std::string filename)
{
  send(client, http_file_[filename], strlen(http_file_[filename]), MSG_NOSIGNAL);
}

/**
 * @brief 客户端超时回调函数 
 * 
 * @param overtime_timer 
 */
void TcpEpollServer::client_overtime_cb(timer_tick::Timer* overtime_timer)
{
  close_client(overtime_timer->fd());
}

} // namespace http_server

