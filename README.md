# BeastServer: 基于C++ beast库实现的支持Http和WebSocket协议请求的异步服务器

## 项目构建

1.进入项目

cd BeastServer

2.创建build文件夹并进入

mkdir build && cd build

3.构建项目并编译

cmake .. && make

4.运行项目
./BeastServer

## 完成功能

1.实现http的get、post请求

<http://127.0.0.1:10000/time>

2.使用C++11智能指针引用技术模拟伪闭包延长连接的生命周期，防止多重析构

3.添加信号捕捉，使服务器安全退出

4.升级http，实现websocket协议echo模式服务器
ws//127.0.0.1:10000

5.asio多线程模式，IOServicePool，一个线程创建一个iocontext，每个iocontext处理多个socket，提高处理请求的效率

todo

6.单例异步日志系统

（7）.添加逻辑单例类处理websocket请求，不再echo模式
