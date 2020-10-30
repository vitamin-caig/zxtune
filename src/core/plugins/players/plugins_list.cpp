/**
* 
* @file
*
* @brief  Player plugins factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <core/plugins/players/plugins_list.h>

namespace ZXTune
{
  void RegisterPlayerPlugins(PlayerPluginsRegistrator& registrator)
  {
    //try TS & AY first
    RegisterTSSupport(registrator);
    RegisterAYSupport(registrator);
    RegisterPT3Support(registrator);
    RegisterPT2Support(registrator);
    RegisterSTCSupport(registrator);
    RegisterST1Support(registrator);
    RegisterST3Support(registrator);
    RegisterASCSupport(registrator);
    RegisterSTPSupport(registrator);
    RegisterTXTSupport(registrator);
    RegisterPSGSupport(registrator);
    RegisterPDTSupport(registrator);
    RegisterCHISupport(registrator);
    RegisterSTRSupport(registrator);
    RegisterDSTSupport(registrator);
    RegisterSQDSupport(registrator);
    RegisterDMMSupport(registrator);
    RegisterPSMSupport(registrator);
    RegisterGTRSupport(registrator);
    RegisterPT1Support(registrator);
    RegisterVTXSupport(registrator);
    RegisterYMSupport(registrator);
    RegisterTFDSupport(registrator);
    RegisterTFCSupport(registrator);
    RegisterSQTSupport(registrator);
    RegisterPSCSupport(registrator);
    RegisterFTCSupport(registrator);
    RegisterCOPSupport(registrator);
    RegisterTFESupport(registrator);
    RegisterXMPPlugins(registrator);
    RegisterSIDPlugins(registrator);
    RegisterET1Support(registrator);
    RegisterAYCSupport(registrator);
    RegisterSPCSupport(registrator);
    RegisterMTCSupport(registrator);
    RegisterGMEPlugins(registrator);
    RegisterAHXSupport(registrator);
    RegisterPSFSupport(registrator);
    RegisterUSFSupport(registrator);
    RegisterGSFSupport(registrator);
    Register2SFSupport(registrator);
    RegisterSDSFSupport(registrator);
    RegisterASAPPlugins(registrator);
    RegisterMP3Plugin(registrator);
    RegisterOGGPlugin(registrator);
    RegisterWAVPlugin(registrator);
    RegisterFLACPlugin(registrator);
    RegisterV2MSupport(registrator);
    RegisterVGMPlugins(registrator);
  }
}
