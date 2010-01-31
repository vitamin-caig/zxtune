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

#include <fstream>
#include <memory>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Extracting first component from complex path
    //! @param path Input path
    //! @param restPart The rest part besides the first component
    //! @return The first component of specified path
    String ExtractFirstPathComponent(const String& path, String& restPart);
    
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

    //! @brief Creates file
    //! @param path Path to the file
    //! @param overwrite Force to overwrite if file already exists
    //! @return File stream
    //! @note Throws exception if file is already exists and overwrite flag is not set
    std::auto_ptr<std::ofstream> CreateFile(const String& path, bool overwrite);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
