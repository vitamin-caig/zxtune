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

namespace ZXTune
{
  class PlayerPluginsRegistrator;
  class ArchivePluginsRegistrator;

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
  void RegisterSIDPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterET1Support(PlayerPluginsRegistrator& registrator);
  void RegisterAYCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSPCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterMTCSupport(PlayerPluginsRegistrator& registrator);
  void RegisterGMEPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterAHXSupport(PlayerPluginsRegistrator& registrator);
  void RegisterPSFSupport(PlayerPluginsRegistrator& registrator);
  void RegisterUSFSupport(PlayerPluginsRegistrator& registrator);
  void RegisterGSFSupport(PlayerPluginsRegistrator& registrator);
  void Register2SFSupport(PlayerPluginsRegistrator& registrator);
  void RegisterNCSFSupport(PlayerPluginsRegistrator& registrator);
  void RegisterSDSFSupport(PlayerPluginsRegistrator& registrator);
  void RegisterASAPPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
  void RegisterMP3Plugin(PlayerPluginsRegistrator& registrator);
  void RegisterOGGPlugin(PlayerPluginsRegistrator& registrator);
  void RegisterWAVPlugin(PlayerPluginsRegistrator& registrator);
  void RegisterFLACPlugin(PlayerPluginsRegistrator& registrator);
  void RegisterV2MSupport(PlayerPluginsRegistrator& registrator);
  void RegisterVGMPlugins(PlayerPluginsRegistrator& registrator);
  void RegisterMPTPlugins(PlayerPluginsRegistrator& registrator);
  void RegisterVGMStreamPlugins(PlayerPluginsRegistrator& registrator, ArchivePluginsRegistrator& archives);

  void RegisterPlayerPlugins(PlayerPluginsRegistrator& registrator);
  void RegisterMultitrackPlayerPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
}  // namespace ZXTune
