#include <io/provider.h>
#include <core/player.h>
#include <core/plugin.h>
#include <core/error_codes.h>
#include <iostream>

#include <boost/bind.hpp>

using namespace ZXTune;

namespace
{
  const unsigned char PSG_DUMP[] = {
    0, 0, 0, 0, 0, 0, 0, 0,//skip
    //offset=8 size=24 crc=1720746500
    'P', 'S', 'G', 0x1a, 1, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xff, 
    0, 0, 1, 1, //0x100
    7, 0xfe,
    0xfd,//end
    
    0, 0, 0, 0, 0, 0, 0, 0,//skip
    //offset=40 size=24 crc=799865041
    'P', 'S', 'G', 0x1a, 1, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xff,
    0, 0xff,
    6, 10,
    7, ~(1 | 8),
    0xfd,
    
    0, 0//skip
  };
    
  class FixedContainer : public IO::DataContainer
  {
  public:
    FixedContainer(const uint8_t* data, std::size_t size)
      : Ptr(data), Len(size)
    {
    }
    
    virtual std::size_t Size() const
    {
      return Len;
    }
    virtual const void* Data() const
    {
      return Ptr;
    }
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Len);
      return DataContainer::Ptr(new FixedContainer(Ptr + offset, size));
    }
  private:
    const uint8_t* const Ptr;
    const std::size_t Len;
  };
  
  inline IO::DataContainer::Ptr CreateContainer(std::size_t offset = 0)
  {
    return IO::DataContainer::Ptr(new FixedContainer(PSG_DUMP + offset, ArraySize(PSG_DUMP) - offset));
  }
  
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cout << Error::AttributesToString(loc, code, text);;
  }
  
  void ShowPluginInfo(const PluginInformation& info)
  {
    std::cout << 
      " Plugin:\n"
      "  Id: " << info.Id << "\n"
      "  Descr: " << info.Description << "\n"
      "  Vers: " << info.Version << "\n"
      "  Caps: 0x" << std::hex << info.Capabilities << std::dec << "\n";
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
    std::cout << "  " << val.first << ": " << boost::apply_visitor(OutVisitor(), val.second) << "\n";
  }
  
  void ShowModuleInfo(const Module::Information& info)
  {
    std::cout << " Module:\n"
      "  Stat.Position: " << info.Statistic.Position << "\n"
      "  Stat.Pattern: " << info.Statistic.Pattern << "\n"
      "  Stat.Line: " << info.Statistic.Line << "\n"
      "  Stat.Frame: " << info.Statistic.Frame << "\n"
      "  Stat.Tempo: " << info.Statistic.Tempo << "\n"
      "  Stat.Channels: " << info.Statistic.Channels << "\n"
      "  Loop: " << info.Loop << "\n"
      "  Channels: " << info.PhysicalChannels << "\n";
    std::for_each(info.Properties.begin(), info.Properties.end(), OutProp);
  }
  
  bool PluginFilter(const PluginInformation& info)
  {
    return info.Id == "PSG";
  }
  
  void PluginLogger(const String& str)
  {
    std::cout << " >" << str << std::endl;
  }
  
  Error PluginCallback(Module::Player::Ptr player, unsigned& count)
  {
    PluginInformation plugInfo;
    player->GetPlayerInfo(plugInfo);
    std::cout << " Plugin: " << plugInfo.Id << std::endl;
    Module::Information modInfo;
    player->GetModuleInformation(modInfo);
    ShowModuleInfo(modInfo);
    ++count;
    return Error();
  }
  
  void Test(bool res, const String& txt, unsigned line)
  {
    if (res)
    {
      std::cout << "Passed test for " << txt << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << txt << " at line " << line << std::endl;
    }
  }
  
  void TestError(const Error& err, Error::CodeType code, const String& txt, unsigned line)
  {
    Test(err == code, txt + ":", line);
    err.WalkSuberrors(ErrOuter);
  }
}

int main()
{
  try
  {
    std::vector<PluginInformation> plugins;
    GetSupportedPlugins(plugins);
    std::cout << "Supported plugins:" << std::endl;
    std::for_each(plugins.begin(), plugins.end(), ShowPluginInfo);
    
    unsigned count = 0;
    DetectParameters detectParams;
    TestError(DetectModules(CreateContainer(8), detectParams, String()), Module::ERROR_INVALID_PARAMETERS, "no callback", __LINE__);
    
    detectParams.Callback = boost::bind(PluginCallback, _1, boost::ref(count));
    TestError(DetectModules(IO::DataContainer::Ptr(), detectParams, String()), Module::ERROR_INVALID_PARAMETERS, "no data", __LINE__);
    
    detectParams.Logger = PluginLogger;
    
    TestError(DetectModules(CreateContainer(), detectParams, "invalid_path"), Module::ERROR_FIND_SUBMODULE, "invalid path", __LINE__);
    
    ThrowIfError(DetectModules(CreateContainer(40), detectParams, String()));
    Test(count == 1, "simple opening", __LINE__);
    detectParams.Filter = PluginFilter;
    count = 0;
    ThrowIfError(DetectModules(CreateContainer(8), detectParams, String()));
    
    Test(count == 0, "filtered opening", __LINE__);
    detectParams.Filter = 0;
    count = 0;
    ThrowIfError(DetectModules(CreateContainer(), detectParams, String()));
    Test(count == 2, "raw scanning opening", __LINE__);
    count = 0;
    ThrowIfError(DetectModules(CreateContainer(8), detectParams, String()));
    Test(count == 2, "raw scanning starting from begin", __LINE__);
    count = 0;
    ThrowIfError(DetectModules(CreateContainer(), detectParams, "8.raw"));
    Test(count == 1, "detect from startpoint", __LINE__);
    count = 0;
    ThrowIfError(DetectModules(CreateContainer(), detectParams, "1.raw"));
    Test(count == 0, "detect from invalid offset", __LINE__);
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
}
