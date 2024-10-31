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

#include <binary/view.h>

namespace Binary
{
  uint32_t Crc32(View data, uint32_t initial = 0);
}