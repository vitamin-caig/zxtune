/**
*
* @file     formats/archived/decoders.h
* @brief    Archived data accessors factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef FORMATS_ARCHIVED_DECODERS_H_DEFINED
#define FORMATS_ARCHIVED_DECODERS_H_DEFINED

//library includes
#include <formats/archived.h>

namespace Formats
{
  namespace Archived
  {
    Decoder::Ptr CreateZipDecoder();
    Decoder::Ptr CreateRarDecoder();
    Decoder::Ptr CreateZXZipDecoder();
    Decoder::Ptr CreateSCLDecoder();
    Decoder::Ptr CreateTRDDecoder();
    Decoder::Ptr CreateHripDecoder();
    Decoder::Ptr CreateAYDecoder();
    Decoder::Ptr CreateLhaDecoder();
    Decoder::Ptr CreateZXStateDecoder();
  }
}

#endif //FORMATS_ARCHIVED_DECODERS_H_DEFINED
