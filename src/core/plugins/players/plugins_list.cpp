/**
 *
 * @file
 *
 * @brief  Player plugins factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/plugins_list.h"

namespace ZXTune
{
  void RegisterAYMPlugins(PlayerPluginsRegistrator& players);
  void RegisterDACPlugins(PlayerPluginsRegistrator& players);
  void RegisterTFMPlugins(PlayerPluginsRegistrator& players);
  void RegisterCOPSupport(PlayerPluginsRegistrator& players);
  void RegisterXMPPlugins(PlayerPluginsRegistrator& players);
  void RegisterSIDPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterSPCSupport(PlayerPluginsRegistrator& players);
  void RegisterMTCSupport(PlayerPluginsRegistrator& players);
  void RegisterGMEPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterXSFPlugins(PlayerPluginsRegistrator& players);
  void RegisterASAPPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterMP3Plugin(PlayerPluginsRegistrator& players);
  void RegisterOGGPlugin(PlayerPluginsRegistrator& players);
  void RegisterWAVPlugin(PlayerPluginsRegistrator& players);
  void RegisterFLACPlugin(PlayerPluginsRegistrator& players);
  void RegisterVGMPlugins(PlayerPluginsRegistrator& players);
  void RegisterMPTPlugins(PlayerPluginsRegistrator& players);
  void RegisterVGMStreamPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);

  void RegisterPlayerPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    RegisterAYMPlugins(players);
    RegisterDACPlugins(players);
    RegisterTFMPlugins(players);
    RegisterCOPSupport(players);
    RegisterXMPPlugins(players);
    RegisterSPCSupport(players);
    RegisterMTCSupport(players);
    RegisterXSFPlugins(players);
    RegisterMP3Plugin(players);
    RegisterOGGPlugin(players);
    RegisterFLACPlugin(players);
    RegisterVGMPlugins(players);
    RegisterMPTPlugins(players);
    RegisterSIDPlugins(players, archives);
    RegisterGMEPlugins(players, archives);
    RegisterASAPPlugins(players, archives);
    RegisterVGMStreamPlugins(players, archives);
    RegisterWAVPlugin(players);  // wav after VGMStream
  }
}  // namespace ZXTune
