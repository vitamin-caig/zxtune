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
  
  void ShowPluginInfo(const PluginInformation& info)
  {
    std::cout << "Plugin:\n"
      "Id: " << info.Id << "\n"
      "Descr: " << info.Description << "\n"
      "Vers: " << info.Version << "\n"
      "Caps: 0x" << std::hex << info.Capabilities << "\n";
  }

  class OutVisitor : public boost::static_visitor<String>
  {
  public:
    String operator()(const Dump& dmp) const
    {
      OutStringStream str;
      str << "<array of size " << dmp.size() << ">";
      return str.str();
    }
    template<class T>
    String operator()(const T& var) const
    {
      OutStringStream str;
      str << var;
      return str.str();
    }
  };

  void OutProp(const ParametersMap::value_type& val)
  {
    std::cout << val.first << ": " << boost::apply_visitor(OutVisitor(), val.second) << "\n";
  }
  
  void ShowModuleInfo(const Module::Information& info)
  {
    std::cout << "Module:\n"
      "Stat.Position: " << info.Statistic.Position << "\n"
      "Stat.Pattern: " << info.Statistic.Pattern << "\n"
      "Stat.Line: " << info.Statistic.Line << "\n"
      "Stat.Frame: " << info.Statistic.Frame << "\n"
      "Stat.Tempo: " << info.Statistic.Tempo << "\n"
      "Stat.Channels: " << info.Statistic.Channels << "\n"
      "Loop: " << info.Loop << "\n"
      "Channels: " << info.PhysicalChannels << "\n"
      "Caps: 0x" << std::hex << info.Capabilities << "\n";
    std::for_each(info.Properties.begin(), info.Properties.end(), OutProp);
  }
  
  bool PluginFilter(const PluginInformation&)
  {
    return true;
  }
  
  void PluginLogger(const String& str)
  {
    std::cout << str << std::endl;
  }
  
  Error PluginCallback(Module::Player::Ptr player)
  {
    PluginInformation plugInfo;
    player->GetPlayerInfo(plugInfo);
    ShowPluginInfo(plugInfo);
    Module::Information modInfo;
    player->GetModuleInformation(modInfo);
    ShowModuleInfo(modInfo);
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
    detectParams.Logger = PluginLogger;
    //detectParams.Filter = PluginFilter;
    detectParams.Callback = PluginCallback;
    ThrowIfError(DetectModules(*data, detectParams, subpath));
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
}
