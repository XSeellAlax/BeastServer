#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <iostream>

using namespace std;

int main()
{
  Json::Value root;
  root["id"] = 1001;
  root["data"] = "hello world";

  // 序列化
  // 结果保存在request中
  std::string request = root.toStyledString();
  std::cout << "request is " << request << std::endl;

  // 反序列化
  Json::Value root2;
  Json::Reader reader;
  // 结果保存在对象root2中
  reader.parse(request, root2);
  std::cout << "msg id is " << root2["id"] << " msg is " << root2["data"] << std::endl;

  return 0;
}