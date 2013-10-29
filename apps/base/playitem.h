/**
* 
* @file
*
* @brief  Playitems support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_holder.h>
#include <io/identifier.h>

Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath);
Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id);
