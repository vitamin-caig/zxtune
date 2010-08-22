/**
*
* @file     detector.h
* @brief    Format detector interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __DETECTOR_H_DEFINED__
#define __DETECTOR_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <string>

//Pattern format
//xx - match byte (hex)
//? - any byte
//+xx+ - skip xx bytes (dec)
bool DetectFormat(const uint8_t* data, std::size_t size, const std::string& pattern);

#endif //__DETECTOR_H_DEFINED__
