/*
  服务器状态信息类
  用于测试，客户端通过请求可获取服务器被请求次数或当前时间戳
*/

#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>
#include <ctime>

#include "Singleton.h"

class ServerStateInfo : public Singleton<ServerStateInfo>
{
  friend class Singleton<ServerStateInfo>;

public:
  // 统计访问次数,调用一次加1
  std::size_t request_count()
  {
    static std::size_t count = 0;
    return ++count;
  }

  // 获取当前时间戳
	std::time_t now()
	{
		return std::time(0);
	}

  ~ServerStateInfo()
  {
    std::cout << "ServerStateInfo Destructor" << std::endl;
  }

private:
  ServerStateInfo() {}
};