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

// common includes
#include <string_type.h>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3]);
}
