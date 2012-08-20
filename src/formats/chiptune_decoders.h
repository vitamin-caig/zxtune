/**
*
* @file     formats/chiptune_decoders.h
* @brief    Chiptune analyzers factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_CHIPTUNE_DECODERS_H_DEFINED__
#define __FORMATS_CHIPTUNE_DECODERS_H_DEFINED__

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
  }
}

#endif //__FORMATS_CHIPTUNE_DECODERS_H_DEFINED__
