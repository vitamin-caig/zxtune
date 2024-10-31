/**
 *
 * @file
 *
 * @brief  Player plugins factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <core/plugins/players/plugins_list.h>

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& players);
  void RegisterAYSupport(PlayerPluginsRegistrator& players);
  void RegisterPSGSupport(PlayerPluginsRegistrator& players);
  void RegisterSTCSupport(PlayerPluginsRegistrator& players);
  void RegisterST1Support(PlayerPluginsRegistrator& players);
  void RegisterST3Support(PlayerPluginsRegistrator& players);
  void RegisterPT2Support(PlayerPluginsRegistrator& players);
  void RegisterPT3Support(PlayerPluginsRegistrator& players);
  void RegisterASCSupport(PlayerPluginsRegistrator& players);
  void RegisterSTPSupport(PlayerPluginsRegistrator& players);
  void RegisterTXTSupport(PlayerPluginsRegistrator& players);
  void RegisterPDTSupport(PlayerPluginsRegistrator& players);
  void RegisterCHISupport(PlayerPluginsRegistrator& players);
  void RegisterSTRSupport(PlayerPluginsRegistrator& players);
  void RegisterDSTSupport(PlayerPluginsRegistrator& players);
  void RegisterSQDSupport(PlayerPluginsRegistrator& players);
  void RegisterDMMSupport(PlayerPluginsRegistrator& players);
  void RegisterPSMSupport(PlayerPluginsRegistrator& players);
  void RegisterGTRSupport(PlayerPluginsRegistrator& players);
  void RegisterPT1Support(PlayerPluginsRegistrator& players);
  void RegisterVTXSupport(PlayerPluginsRegistrator& players);
  void RegisterYMSupport(PlayerPluginsRegistrator& players);
  void RegisterTFDSupport(PlayerPluginsRegistrator& players);
  void RegisterTFCSupport(PlayerPluginsRegistrator& players);
  void RegisterSQTSupport(PlayerPluginsRegistrator& players);
  void RegisterPSCSupport(PlayerPluginsRegistrator& players);
  void RegisterFTCSupport(PlayerPluginsRegistrator& players);
  void RegisterCOPSupport(PlayerPluginsRegistrator& players);
  void RegisterTFESupport(PlayerPluginsRegistrator& players);
  void RegisterXMPPlugins(PlayerPluginsRegistrator& players);
  void RegisterSIDPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterET1Support(PlayerPluginsRegistrator& players);
  void RegisterAYCSupport(PlayerPluginsRegistrator& players);
  void RegisterSPCSupport(PlayerPluginsRegistrator& players);
  void RegisterMTCSupport(PlayerPluginsRegistrator& players);
  void RegisterGMEPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterAHXSupport(PlayerPluginsRegistrator& players);
  void RegisterPSFSupport(PlayerPluginsRegistrator& players);
  void RegisterUSFSupport(PlayerPluginsRegistrator& players);
  void RegisterGSFSupport(PlayerPluginsRegistrator& players);
  void Register2SFSupport(PlayerPluginsRegistrator& players);
  void RegisterNCSFSupport(PlayerPluginsRegistrator& players);
  void RegisterSDSFSupport(PlayerPluginsRegistrator& players);
  void RegisterASAPPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterMP3Plugin(PlayerPluginsRegistrator& players);
  void RegisterOGGPlugin(PlayerPluginsRegistrator& players);
  void RegisterWAVPlugin(PlayerPluginsRegistrator& players);
  void RegisterFLACPlugin(PlayerPluginsRegistrator& players);
  void RegisterV2MSupport(PlayerPluginsRegistrator& players);
  void RegisterVGMPlugins(PlayerPluginsRegistrator& players);
  void RegisterMPTPlugins(PlayerPluginsRegistrator& players);
  void RegisterVGMStreamPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);

  void RegisterPlayerPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    // try TS & AY first
    RegisterTSSupport(players);
    RegisterAYSupport(players);
    RegisterPT3Support(players);
    RegisterPT2Support(players);
    RegisterSTCSupport(players);
    RegisterST1Support(players);
    RegisterST3Support(players);
    RegisterASCSupport(players);
    RegisterSTPSupport(players);
    RegisterTXTSupport(players);
    RegisterPSGSupport(players);
    RegisterPDTSupport(players);
    RegisterCHISupport(players);
    RegisterSTRSupport(players);
    RegisterDSTSupport(players);
    RegisterSQDSupport(players);
    RegisterDMMSupport(players);
    RegisterPSMSupport(players);
    RegisterGTRSupport(players);
    RegisterPT1Support(players);
    RegisterVTXSupport(players);
    RegisterYMSupport(players);
    RegisterTFDSupport(players);
    RegisterTFCSupport(players);
    RegisterSQTSupport(players);
    RegisterPSCSupport(players);
    RegisterFTCSupport(players);
    RegisterCOPSupport(players);
    RegisterTFESupport(players);
    RegisterXMPPlugins(players);
    RegisterET1Support(players);
    RegisterAYCSupport(players);
    RegisterSPCSupport(players);
    RegisterMTCSupport(players);
    RegisterAHXSupport(players);
    RegisterPSFSupport(players);
    RegisterUSFSupport(players);
    RegisterGSFSupport(players);
    Register2SFSupport(players);
    RegisterNCSFSupport(players);
    RegisterSDSFSupport(players);
    RegisterMP3Plugin(players);
    RegisterOGGPlugin(players);
    RegisterFLACPlugin(players);
    RegisterVGMPlugins(players);
    RegisterMPTPlugins(players);
    RegisterSIDPlugins(players, archives);
    RegisterGMEPlugins(players, archives);
    RegisterASAPPlugins(players, archives);
    RegisterVGMStreamPlugins(players, archives);
    RegisterV2MSupport(players);
    RegisterWAVPlugin(players);  // wav after VGMStream
  }
}  // namespace ZXTune
