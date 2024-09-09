#include <iostream>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory.h>

#include "./Connection/Connection.h"
#include "./IOServicePool/AsioIOServicePool.h"

/*
	服务器类

	监听客户端，接受到一个连接就创建一个Connection类
*/
void Server(boost::asio::ip::tcp::acceptor &acceptor, boost::asio::ip::tcp::socket &socket, boost::asio::io_context& ioc)
{
	// 为每一个连接创建一个从服务池中的ioservice
	auto &pool_ioc = AsioIOServicePool::GetInstance()->GetIOService();


	acceptor.async_accept(socket,
												[&](boost::beast::error_code ec)
												{
													if (!ec)
														std::make_shared<Connection>(std::move(socket), pool_ioc)->start();
													Server(acceptor, socket, ioc);
												});
}

int main()
{
	try
	{
		auto const address = boost::asio::ip::make_address("127.0.0.1");
		unsigned short port = static_cast<unsigned short>(10000);

		std::cout << "Server start on " << address << ", port is " << port << std::endl;

		// 此pool用于已经监听到的socket处理读写，在main中创建的iocontext只负责accept
		auto pool = AsioIOServicePool::GetInstance();

		boost::asio::io_context ioc{1};

		// 注册捕捉信号，捕获到该信号服务器就安全的退出
		// SIGINT：Ctrl + C
    // SIGTERM：kill
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

		// 异步等待，捕获到信号就触发
		signals.async_wait([&ioc, &pool](auto, auto) {
			ioc.stop();
			std::cout << "Server stop" << std::endl;

			pool->Stop();
			std::cout << "ServicePool stop" << std::endl;
		});


		boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};
		boost::asio::ip::tcp::socket socket{ioc};
		Server(acceptor, socket, ioc);

		ioc.run();
	}
	catch (std::exception const &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}