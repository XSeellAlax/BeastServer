/*
  连接类
  一个新socket对应一个连接类用于通信
*/

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <cstdlib>
#include <queue>
#include <mutex>

// 重命名名字空间
namespace beast = boost::beast;		// from <boost/beast.hpp>
namespace http = beast::http;			// from <boost/beast/http.hpp>
namespace net = boost::asio;			// from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

using namespace boost::beast;
using namespace boost::beast::websocket;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
  Connection(tcp::socket socket, boost::asio::io_context& ioc);

  // 开始函数
  void start();

  // 处理websocket错误
  void handle_error(boost::system::error_code ec);

  // 转换成websocket
  void convert_websocket( tcp::socket&& socket);

  // 异步写入消息队列
  void async_send_message(const std::string& message);

private:
  // 读请求
  void read_request();

  // 处理请求
	void process_request();

  // 处理get响应
	void create_get_response();

  //// 处理post响应
	void create_post_response();

  // 发送响应
	void write_response();

  // 超时检查
  void check_deadline();

  // 关闭定时器
  void cancel_deadline();

  tcp::socket _socket; // 用于通信的socket
  beast::flat_buffer _buffer{8192}; // 用于存储请求的缓冲区
  http::request<http::dynamic_body> _request; // 存储请求信息
  http::response<http::dynamic_body> _response; // 存储响应信息
  net::steady_timer _deadline{
    _socket.get_executor(), std::chrono::seconds(5)
  }; // 定时器5s

  std::unique_ptr<stream<tcp_stream>> ws_ptr_; // 管理websocket

  // 升级http协议
  void upgrade_websocket(const tcp::socket&& socket, std::size_t bytes_transferred);
	
  // websocket读
  void websocket_async_read();

  // 发送消息
	void async_write(const std::string& message);

  // 读缓存
	flat_buffer _read_buffer;

  // websocket发送队列
	std::queue<std::string> _send_que;
	std::mutex  _mutex;
	boost::asio::strand<boost::asio::io_context::executor_type> _strand; // 保证线程安全
	boost::asio::io_context& _ioc;
};