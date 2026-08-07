/**
 *
 * @file
 *
 * @brief  TR-DOS utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"

namespace Formats::Archived::TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3]);
}
