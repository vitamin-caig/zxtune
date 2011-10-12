/**
*
* @file     formats/archived_decoders.h
* @brief    Archived data accessors factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_ARCHIVED_DECODERS_H_DEFINED__
#define __FORMATS_ARCHIVED_DECODERS_H_DEFINED__

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
  }
}

#endif //__FORMATS_ARCHIVED_DECODERS_H_DEFINED__
