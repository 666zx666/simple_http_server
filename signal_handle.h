 /**
 * @file signal_handle.h
 * @author zX
 * @brief 
 * @version 0.1
 * @date 2019-10-24
 *  
 * @copyright Copyright (c) 2019
 * 
 */
#ifndef SIGNAL_HANDLE_H_
#define SIGNAL_HANDLE_H_

#include <signal.h>
#include <boost/noncopyable.hpp>

namespace http_server
{

class SignalHandle : public boost::noncopyable
{
public:
  explicit SignalHandle(){}

  void set_sigpipe(){}

private:

};

} // http_server



#endif