#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <cstdlib>

void printStack()
{
  pid_t myPid = getpid();
  std::string pstackCommand = "echo \"thread backtrace all\" | lldb -p ";
  std::stringstream ss;
  ss << myPid;
  pstackCommand += ss.str();
  system(pstackCommand.c_str());
}
