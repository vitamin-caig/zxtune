#include <io/provider.h>
#include <core/player.h>
#include <core/plugin.h>
#include <iostream>

using namespace ZXTune;

namespace
{
  const char PSG_TEST[] = "../cmdline/samples/psg/Illusion.psg";

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("%1%\n\nCode: %2%\nAt: %3%\n--------\n") % text % code % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  bool PluginFilter(const PluginInformation&)
  {
    return true;
  }
  
  Error PluginCallback(const String& name, Module::Player::Ptr player)
  {
    std::cout << "Processed " << name << std::endl;
    return Error();
  }
}

int main()
{
  try
  {
    String subpath;
    IO::OpenDataParameters openParams;
    IO::DataContainer::Ptr data;
    ThrowIfError(OpenData(PSG_TEST, openParams, data, subpath));
    
    DetectParameters detectParams;
    detectParams.Filter = PluginFilter;
    detectParams.Subpath = PSG_TEST;
    detectParams.Callback = PluginCallback;
    ThrowIfError(DetectModules(*data, detectParams));
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
}
