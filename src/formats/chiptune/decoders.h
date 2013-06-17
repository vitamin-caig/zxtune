/**
*
* @file     formats/chiptune/decoders.h
* @brief    Chiptune analyzers factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef FORMATS_CHIPTUNE_DECODERS_H_DEFINED
#define FORMATS_CHIPTUNE_DECODERS_H_DEFINED

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
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
    Decoder::Ptr CreateProSoundMakerCompiledDecoder();
    Decoder::Ptr CreateGlobalTrackerDecoder();
    Decoder::Ptr CreateProTracker1Decoder();
    Decoder::Ptr CreateVTXDecoder();
    Decoder::Ptr CreateYMDecoder();
    Decoder::Ptr CreateTFDDecoder();
    Decoder::Ptr CreateTFCDecoder();
    Decoder::Ptr CreateVortexTracker2Decoder();
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
  }
}

#endif //FORMATS_CHIPTUNE_DECODERS_H_DEFINED
