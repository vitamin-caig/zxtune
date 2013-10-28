#include <apps/version/api.h>
#include <iostream>

namespace Text
{
  extern const Char PROGRAM_NAME[] = {'T', 'e', 's', 't', 0};
}

int main()
{
  std::cout << GetProgramVersionString() << std::endl;
}