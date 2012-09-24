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
//std includes
#include <fstream>
#include <memory>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Converting filename to single-char string
    //! @param str Input filename
    //! @return Equivalent filename in single-char format
    std::string ConvertToFilename(const String& str);

    //! @brief Extracting last component from complex path (usually filename)
    //! @param path Input path
    //! @param restPart The rest part before the last component
    //! @return The last component of specified path
    String ExtractLastPathComponent(const String& path, String& restPart);

    //! @brief Appending component to existing path
    //! @param path1 Base path
    //! @param path2 Additional component
    //! @return Merged path
    String AppendPath(const String& path1, const String& path2);

    //! @brief Converting any string to suitable filename
    //! @param input Source string
    //! @param replacing Character to replace invalid symbols (0 to simply skip)
    //! @return Path string
    String MakePathFromString(const String& input, Char replacing);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
