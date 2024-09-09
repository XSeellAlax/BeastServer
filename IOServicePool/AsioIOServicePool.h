/*
	多线程模式，并行多个io_service

	在实现IO服务池的时候，防止在还没有注册事件时候io_context直接返回，添加使用了boost::asio::io_context::work；
*/

#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "../ServerStateInfo/Singleton.h"

class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::io_context::work;
	using WorkPtr = std::unique_ptr<Work>;

	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

	// round-robin轮询方式获得io_service
	boost::asio::io_context& GetIOService();

	// io_service服务停止
	void Stop();

private:
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
	std::vector<IOService> _ioServices;
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;
	std::size_t _nextIOService;
};

