/**
*
* @file     crc.h
* @brief    CRC calculating functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CRC_H_DEFINED__
#define __CRC_H_DEFINED__

//common includes
#include <types.h>

uint32_t Crc32(const uint8_t* buf, std::size_t len, uint32_t initial = 0);

#endif //__CRC_H_DEFINED__
