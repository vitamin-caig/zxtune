/**
*
* @file     core/module_attrs.h
* @brief    Module attributes names
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_ATTRS_H_DEFINED__
#define __CORE_MODULE_ATTRS_H_DEFINED__

#include <char_type.h>

namespace ZXTune
{
  namespace Module
  {
    //@{
    //! @name Built-in attributes
    
    //! %Module type attribute @see PluginInformation#Id
    const Char ATTR_TYPE[] = {'T', 'y', 'p', 'e', '\0'};
    //! %Module containers chain
    const Char ATTR_CONTAINER[] = {'C', 'o', 'n', 't', 'a', 'i', 'n', 'e', 'r', '\0'};
    //! %Module path in top-level data dump
    const Char ATTR_SUBPATH[] = {'S', 'u', 'b', 'p', 'a', 't', 'h', '\0'};
    //! %Module author
    const Char ATTR_AUTHOR[] = {'A', 'u', 't', 'h', 'o', 'r', '\0'};
    //! %Module title
    const Char ATTR_TITLE[] = {'T', 'i', 't', 'l', 'e', '\0'};
    //! %Module program produced at
    const Char ATTR_PROGRAM[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', '\0'};
    //! %Module computer (hardware platform)
    const Char ATTR_COMPUTER[] = {'C', 'o', 'm', 'p', 'u', 't', 'e', 'r', '\0'};
    //! %Module creating date
    const Char ATTR_DATE[] = {'D', 'a', 't', 'e', '\0'};
    //! %Module comment
    const Char ATTR_COMMENT[] = {'C', 'o', 'm', 'm', 'e', 'n', 't', '\0'};
    //! %Module structure warnings
    const Char ATTR_WARNINGS[] = {'W', 'a', 'r', 'n', 'i', 'n', 'g', 's', '\0'};
    //! %Module structure warnings count
    const Char ATTR_WARNINGS_COUNT[] = {'W', 'a', 'r', 'n', 'i', 'n', 'g', 's', 'C', 'o', 'u', 'n', 't', '\0'};
    //! Raw module data crc32 checksum
    const Char ATTR_CRC[] = {'C', 'R', 'C', '\0'};
    //! Raw module size in bytes
    const Char ATTR_SIZE[] = {'S', 'i', 'z', 'e', '\0'};
    //@}
    
    //@{
    //! @name External attributes
    
    //! Module's input data filename
    const Char ATTR_FILENAME[] = {'F', 'i', 'l', 'e', 'n', 'a', 'm', 'e', '\0'};
    //! Path of module's input data
    const Char ATTR_PATH[] = {'P', 'a', 't', 'h', '\0'};
    //! Full path of module including path of input data and subpath
    const Char ATTR_FULLPATH[] = {'F', 'u', 'l', 'l', 'p', 'a', 't', 'h', '\0'};
    //@}
  }
}

#endif //__CORE_MODULE_ATTRS_H_DEFINED__
