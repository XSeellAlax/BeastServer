#include "./ServerStateInfo/ServerStateInfo.h"

int main()
{
  auto s = ServerStateInfo::GetInstance();  
  std::cout << s->request_count() << std::endl;

  return 0;
}