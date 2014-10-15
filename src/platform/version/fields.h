/**
* 
* @file
*
* @brief Version fields factory
*
* @author vitamin.caig@gmail.com
*
**/


#pragma once

//library includes
#include <strings/fields.h>

namespace Platform
{
  namespace Version
  {
    std::auto_ptr<Strings::FieldsSource> CreateVersionFieldsSource();
  }
}
