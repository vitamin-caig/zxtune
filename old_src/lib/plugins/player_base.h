#ifndef __PLAYER_BASE_H_DEFINED__
#define __PLAYER_BASE_H_DEFINED__

#include <player.h>

namespace ZXTune
{
  class PlayerBase : public ModulePlayer
  {
  public:
    explicit PlayerBase(const String& filename);

    virtual void GetModuleInfo(Module::Information& info) const;
    virtual void Convert(const Conversion::Parameter& param, Dump& dst) const;
  protected:
    void FillProperties(const String& tracker, const String& author, const String& title, 
      const uint8_t* data, std::size_t size);
  protected:
    Dump RawData;
    Module::Information Information;
  };
}

#endif //__PLAYER_BASE_H_DEFINED__
