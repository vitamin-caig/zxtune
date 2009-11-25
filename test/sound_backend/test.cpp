#include <tools.h>
#include <src/sound/backend.h>
#include <src/sound/error_codes.h>

#include <iostream>
#include <iomanip>

namespace
{
  using namespace ZXTune::Sound;

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2$#x\n\tAt: %3%\n\t--------\n") % text % code % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      e.WalkSuberrors(ErrOuter);
    }
    return e;
  }

  void ShowBackend(const Backend::Info& info)
  {
    std::cout << "Backend:\n"
    " Id: " << info.Id << "\n"
    " Version: " << info.Version << "\n"
    " Description: " << info.Description << "\n"
    "---------------------\n";
  }
}

int main()
{
  using namespace ZXTune::Sound;
  
  try
  {
    std::cout << "---- Test for backends enumerating ---" << std::endl;
    std::vector<Backend::Info> infos;
    EnumerateBackends(infos);
    std::for_each(infos.begin(), infos.end(), ShowBackend);
  }
  catch (const Error& e)
  {
    std::cout << " Failed\n";
    e.WalkSuberrors(ErrOuter);
  }
}
