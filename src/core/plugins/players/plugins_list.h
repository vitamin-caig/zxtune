/**
* 
* @file
*
* @brief  Player plugins factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include <core/plugins/registrator.h>

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator);
  void RegisterAYSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPSGSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSTCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterST1Support(PlayerPluginsRegistrator& registrator);
  void RegisterST3Support(PlayerPluginsRegistrator& registrator);
  void RegisterPT2Support(PlayerPluginsRegistrator& registrator);
  void RegisterPT3Support(PlayerPluginsRegistrator& registrator);
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSTPSupport(PlayerPluginsRegistrator& registrator);
  void RegisterTXTSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPDTSupport(PlayerPluginsRegistrator& registrator);
  void RegisterCHISupport(PlayerPluginsRegistrator& registrator);
  void RegisterSTRSupport(PlayerPluginsRegistrator& registrator);
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator);
  void RegisterDMMSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPSMSupport(PlayerPluginsRegistrator& registrator);
  void RegisterGTRSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPT1Support(PlayerPluginsRegistrator& registrator);
  void RegisterVTXSupport(PlayerPluginsRegistrator& registrator);
  void RegisterYMSupport(PlayerPluginsRegistrator& registrator);
  void RegisterTFDSupport(PlayerPluginsRegistrator& registrator);
  void RegisterTFCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSQTSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPSCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterFTCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterCOPSupport(PlayerPluginsRegistrator& registrator);
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator);
  void RegisterXMPPlugins(PlayerPluginsRegistrator& registrator);

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
  }
}
