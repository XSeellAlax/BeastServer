#include "Connection.h"

#include "../ServerStateInfo/ServerStateInfo.h"
auto serverStateInfo = ServerStateInfo::GetInstance();

Connection::Connection(tcp::socket socket, boost::asio::io_context& ioc)
    : _socket(std::move(socket)), ws_ptr_(nullptr), _ioc(ioc),_strand(ioc.get_executor())
{
  std::cout << "one new connection" << std::endl;
}

// 处理websocket错误码
void Connection::handle_error(boost::system::error_code ec)
{
	std::cerr << "WebSocket error: " << ec.message() << "\n";
	// Do something to handle the error, such as logging or shutting down the connection
}

// 转换成websocket连接
void Connection::convert_websocket(tcp::socket&& socket) {
	ws_ptr_.reset(new stream<tcp_stream>(std::move(socket)));
}

// 关闭定时器
void Connection::cancel_deadline() {
	_deadline.cancel();
}

// websocket读
void Connection::websocket_async_read() {
	auto self = shared_from_this();
	// Read a complete message into the buffer's input area asynchronously
	self->ws_ptr_->async_read(self->_read_buffer, boost::asio::bind_executor(_strand, [self](boost::system::error_code ec, std::size_t bytes_transferred)
		{
			try {
				if (!ec)
				{
					// Set text mode if the received message was also text,
					// otherwise binary mode will be set.
					self->ws_ptr_->text(self->ws_ptr_->got_text());

					// Echo the received message back to the peer. If the received
					// message was in text mode, the echoed message will also be
					// in text mode, otherwise it will be in binary mode.
					const auto& recv_data = boost::beast::buffers_to_string(self->_read_buffer.data());
					std::cout << "websocket receive data is " << recv_data << std::endl;
					self->async_send_message(recv_data);

					// Discard all of the bytes stored in the dynamic buffer.
					self->_read_buffer.consume(self->_read_buffer.size());
					self->websocket_async_read();
				}
				else
				{
					self->handle_error(ec);
					// An error occurred while reading from the WebSocket connection.
					//self->ws_ptr_->close(websocket::close_code::none);
					get_lowest_layer(*(self->ws_ptr_)).close();
				}
			}
			catch (std::exception const& e)
			{
				std::cerr << "Error: " << e.what() << std::endl;
				return;
			}
		})
		);
}

// 发送消息
void Connection::async_write(const std::string &message) {
	auto self = shared_from_this();
	self->ws_ptr_->async_write(boost::asio::buffer(message.c_str(), message.length()), boost::asio::bind_executor(_strand,
		[self](boost::system::error_code ec, std::size_t bytes_transferred)
		{
			try {
				if (!ec)
				{
					std::string send_msg;
					// Message sent successfully
					{
						std::lock_guard<std::mutex> lock(self->_mutex);
						self->_send_que.pop();
						if (self->_send_que.empty()) {
							return;
						}

						send_msg = self->_send_que.front();
					}
					self->async_write(send_msg);
				}
				else
				{
					// An error occurred while sending the message
					self->handle_error(ec);
					get_lowest_layer(*(self->ws_ptr_)).close();
				}
			}
			catch (std::exception const& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return;
			}
		}));
}

// 异步写入消息队列
void Connection::async_send_message(const std::string &message)
{
	size_t que_size = 0;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		que_size = _send_que.size();
		_send_que.push(message);
	}

	if (que_size > 0) {
		return;
	}

	async_write(message);
}

// 升级http协议
void Connection::upgrade_websocket(const tcp::socket&& socket, std::size_t bytes_transferred){
	auto self = shared_from_this();
	// Construct the stream, transferring ownership of the socket
	cancel_deadline();
	convert_websocket(std::move(_socket));
	// Clients SHOULD NOT begin sending WebSocket
	// frames until the server has provided a response.
	BOOST_ASSERT(_read_buffer.size() == 0);

	// Accept the upgrade request
	// Accept the upgrade request asynchronously
	self->ws_ptr_->async_accept(self->_request,
		[self](boost::system::error_code ec)
		{
			if (!ec)
			{
				self->websocket_async_read();
			}
			else
			{
				// An error occurred while accepting the WebSocket connection.
				self->handle_error(ec);
				get_lowest_layer(*(self->ws_ptr_)).close();
			}
		});
}

// 发送响应
void Connection::write_response()
{
  auto self = shared_from_this();

  // 消息长度
  _response.content_length(_response.body().size());

  http::async_write(
      _socket,
      _response,
      [self](beast::error_code ec, std::size_t)
      {
        // 关闭服务器的发送端
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);

        // 关闭定时器
        self->_deadline.cancel();
      });
}

// 处理post响应
void Connection::create_post_response()
{
  if (_request.target() == "/email")
  {
    // 取出body
    auto &body = this->_request.body();

    // 取出body内容
    auto body_str = boost::beast::buffers_to_string(body.data());
    std::cout << "receive body is " << body_str << std::endl;
    this->_response.set(http::field::content_type, "text/json");

    // 解析json数据
    Json::Value root;
    Json::Reader reader;

    // 存储解析后的数据
    Json::Value src_root;
    bool parse_success = reader.parse(body_str, src_root);
    if (!parse_success)
    {
      std::cout << "Failed to parse JSON data!" << std::endl;
      root["error"] = 1001;
      std::string jsonstr = root.toStyledString();
      beast::ostream(this->_response.body()) << jsonstr;
      return;
    }

    auto email = src_root["email"].asString();
    std::cout << "email is " << email << std::endl;

    root["error"] = 0;
    root["email"] = src_root["email"];
    root["msg"] = "recevie email post success";
    std::string jsonstr = root.toStyledString();
    beast::ostream(this->_response.body()) << jsonstr;
  }
  else
  {
    _response.result(http::status::not_found);
    _response.set(http::field::content_type, "text/plain");
    beast::ostream(_response.body()) << "File not found\r\n";
  }
}

// 处理get响应
void Connection::create_get_response()
{
  // 请求的路由
  // 127.0.0.1:10000/count
  if (_request.target() == "/count")
  {
    _response.set(http::field::content_type, "text/html");
    beast::ostream(_response.body())
        << "<html>\n"
        << "<head><title>Request count</title></head>\n"
        << "<body>\n"
        << "<h1>Request count</h1>\n"
        << "<p>There have been "
        << serverStateInfo->request_count()
        << " requests so far.</p>\n"
        << "</body>\n"
        << "</html>\n";
  }
  else if (_request.target() == "/time")
  {
    _response.set(http::field::content_type, "text/html");
    beast::ostream(_response.body())
        << "<html>\n"
        << "<head><title>Current time</title></head>\n"
        << "<body>\n"
        << "<h1>Current time</h1>\n"
        << "<p>The current time is "
        << serverStateInfo->now()
        << " seconds since the epoch.</p>\n"
        << "</body>\n"
        << "</html>\n";
  }
  else
  {
    _response.result(http::status::not_found);
    _response.set(http::field::content_type, "text/plain");
    beast::ostream(_response.body()) << "File not found\r\n";
  }
}

// 处理请求
void Connection::process_request()
{
  // 响应的版本
  _response.version(_request.version());

  // 短链接
  _response.keep_alive(false);

  // 判断请求的类型
  switch (_request.method())
  {
  case http::verb::get:
    _response.result(http::status::ok);
    _response.set(http::field::server, "Beast");

    // 处理get响应
    create_get_response();
    break;

  case http::verb::post:
    _response.result(http::status::ok);
    _response.set(http::field::server, "Beast");
    create_post_response();
    break;

  default:
    // 设置状态
    _response.result(http::status::bad_request);

    // 设置content类型
    _response.set(http::field::content_type, "text/plain");

    // 向响应的body里面写数据
    beast::ostream(_response.body())
        << "Invalid request-method '"
        << std::string(_request.method_string())
        << "'";
    break;
  }

  // 发送响应
  write_response();
}

// 读请求
void Connection::read_request()
{
  auto self = shared_from_this();

  beast::http::async_read(
      _socket,
      _buffer,
      _request,
      [self](beast::error_code ec,
             std::size_t bytes_transferred)
      {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
				std::cout << "async_read error ! msg is : " << ec.message() << std::endl;
				return;
			  }
			  if (websocket::is_upgrade(self->_request)) {
        // 处理websocket请求
				self->upgrade_websocket(std::move(self->_socket), bytes_transferred);
			  }
			  else {
				// 处理http请求
				self->process_request();
			  }
      });
}

// 检查是否超时
void Connection::check_deadline()
{
  auto self = shared_from_this();

  // 调用self,引用计数加1
  _deadline.async_wait(
      [self](beast::error_code ec)
      {
        if (!ec)
        {
          // Close socket to cancel any outstanding operation.
          std::cout << "timeout" << std::endl;
          self->_socket.close(ec);
        }
      });
}

// 开始函数
void Connection::start()
{
  // 读请求
  read_request();

  // 超时检测
  check_deadline();
}