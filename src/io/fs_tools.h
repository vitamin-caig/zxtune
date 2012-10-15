/**
*
* @file      io/fs_tools.h
* @brief     Different io-related tools declaration
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __IO_FS_TOOLS_H_DEFINED__
#define __IO_FS_TOOLS_H_DEFINED__

//common includes
#include <types.h>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Converting any string to suitable filename
    //! @param input Source string
    //! @param replacing Character to replace invalid symbols (0 to simply skip)
    //! @return Path string
    String MakePathFromString(const String& input, Char replacing);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
