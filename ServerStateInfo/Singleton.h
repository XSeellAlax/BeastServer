/*
  单例模板类
*/

#pragma once
#include <memory>
#include <mutex>
#include <iostream>

// 模板类T
template <typename T>

class Singleton {

protected:
    Singleton() = default;
    
    // 删除拷贝构造
    Singleton(const Singleton<T>&) = delete;
    
    // 删除拷贝赋值
    Singleton& operator=(const Singleton<T>& st) = delete;

    static std::shared_ptr<T> _instance;
    
public:
    // 返回T类型的单例类
    static std::shared_ptr<T> GetInstance() {
        // std::once_flag保证只会在第一次调用时初始化
        static std::once_flag s_flag;
        
        // 根据std::once_flag来调用构造函数，线程安全
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
            });

        return _instance;
    }
    
    // 打印地址，测试单例
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }
    
    ~Singleton() {
        std::cout << "Base Singleton Destructor" << std::endl;
    }
};

// _instance初始化
// 编译时候不会检测模板类，在被实例化时才初始化，运行下面代码
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;