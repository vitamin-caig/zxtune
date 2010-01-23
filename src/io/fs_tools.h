/**
*
* @file      io/fs_tools.h
* @brief     Different io-related tools declaration
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __IO_FS_TOOLS_H_DEFINED__
#define __IO_FS_TOOLS_H_DEFINED__

#include <string_type.h>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Extracting first component from complex path
    //! @param path Input path
    //! @param restPart The rest part besides the first component
    //! @return The first component of specified path
    String ExtractFirstPathComponent(const String& path, String& restPart);
    
    //! @brief Appending component to existing path
    //! @param path1 Base path
    //! @param path2 Additional component
    //! @return Merged path
    String AppendPath(const String& path1, const String& path2);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
