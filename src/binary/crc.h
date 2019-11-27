/**
*
* @file
*
* @brief  CRC calculating functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data_view.h>

namespace Binary
{
  uint32_t Crc32(DataView data, uint32_t initial = 0);
}