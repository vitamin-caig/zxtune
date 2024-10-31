/**
 *
 * @file
 *
 * @brief  Chiptune decoders factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  Decoder::Ptr CreatePSGDecoder();
  Decoder::Ptr CreateDigitalStudioDecoder();
  Decoder::Ptr CreateSoundTrackerDecoder();
  Decoder::Ptr CreateSoundTrackerCompiledDecoder();
  Decoder::Ptr CreateSoundTracker3Decoder();
  Decoder::Ptr CreateSoundTrackerProCompiledDecoder();
  Decoder::Ptr CreateASCSoundMaster0xDecoder();
  Decoder::Ptr CreateASCSoundMaster1xDecoder();
  Decoder::Ptr CreateProTracker2Decoder();
  Decoder::Ptr CreateProTracker3Decoder();
  Decoder::Ptr CreateVortexTracker2Decoder();
  Decoder::Ptr CreateProSoundMakerCompiledDecoder();
  Decoder::Ptr CreateGlobalTrackerDecoder();
  Decoder::Ptr CreateProTracker1Decoder();
  Decoder::Ptr CreatePackedYMDecoder();
  Decoder::Ptr CreateYMDecoder();
  Decoder::Ptr CreateVTXDecoder();
  Decoder::Ptr CreateTFDDecoder();
  Decoder::Ptr CreateTFCDecoder();
  Decoder::Ptr CreateChipTrackerDecoder();
  Decoder::Ptr CreateSampleTrackerDecoder();
  Decoder::Ptr CreateProDigiTrackerDecoder();
  Decoder::Ptr CreateSQTrackerDecoder();
  Decoder::Ptr CreateProSoundCreatorDecoder();
  Decoder::Ptr CreateFastTrackerDecoder();
  Decoder::Ptr CreateETrackerDecoder();
  Decoder::Ptr CreateSQDigitalTrackerDecoder();
  Decoder::Ptr CreateTFMMusicMaker05Decoder();
  Decoder::Ptr CreateTFMMusicMaker13Decoder();
  Decoder::Ptr CreateDigitalMusicMakerDecoder();
  Decoder::Ptr CreateTurboSoundDecoder();
  Decoder::Ptr CreateExtremeTracker1Decoder();
  Decoder::Ptr CreateAYCDecoder();
  Decoder::Ptr CreateSPCDecoder();
  Decoder::Ptr CreateMultiTrackContainerDecoder();
  Decoder::Ptr CreateAYEMULDecoder();
  Decoder::Ptr CreateVideoGameMusicDecoder();
  Decoder::Ptr CreateGYMDecoder();
  Decoder::Ptr CreateAbyssHighestExperienceDecoder();
  Decoder::Ptr CreateKSSDecoder();
  Decoder::Ptr CreateHivelyTrackerDecoder();
  Decoder::Ptr CreatePSFDecoder();
  Decoder::Ptr CreatePSF2Decoder();
  Decoder::Ptr CreateUSFDecoder();
  Decoder::Ptr CreateGSFDecoder();
  Decoder::Ptr Create2SFDecoder();
  Decoder::Ptr CreateNCSFDecoder();
  Decoder::Ptr CreateSSFDecoder();
  Decoder::Ptr CreateDSFDecoder();
  Decoder::Ptr CreateRasterMusicTrackerDecoder();
  Decoder::Ptr CreateMP3Decoder();
  Decoder::Ptr CreateOGGDecoder();
  Decoder::Ptr CreateWAVDecoder();
  Decoder::Ptr CreateFLACDecoder();
  Decoder::Ptr CreateV2MDecoder();
  Decoder::Ptr CreateSound98Decoder();
}  // namespace Formats::Chiptune
