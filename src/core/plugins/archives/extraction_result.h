/*
Abstract:
  Archive extraction result interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ARCHIVE_EXTRACTION_RESULT_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVE_EXTRACTION_RESULT_H_DEFINED__

//local includes
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/packed.h>
#include <io/container.h>

namespace ZXTune
{
  ArchiveExtractionResult::Ptr ExtractDataFromArchive(const Formats::Packed::Decoder& decoder, IO::DataContainer::Ptr data);
}

#endif //__CORE_PLUGINS_ARCHIVE_EXTRACTION_RESULT_H_DEFINED__
