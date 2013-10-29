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

//common includes
#include <types.h>

uint32_t Crc32(const uint8_t* buf, std::size_t len, uint32_t initial = 0);
