/**
* 
* @file
*
* @brief Convert operation factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "operations.h"
//library includes
#include <sound/service.h>

namespace Playlist
{
  namespace Item
  {
    //export
    class ConversionResultNotification : public Playlist::TextNotification
    {
    public:
      typedef boost::shared_ptr<ConversionResultNotification> Ptr;

      virtual void AddSucceed() = 0;
      virtual void AddFailedToOpen(const String& path) = 0;
      virtual void AddFailedToConvert(const String& path, const Error& err) = 0;
    };

    TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Sound::Service::Ptr service, ConversionResultNotification::Ptr result);

    TextResultOperation::Ptr CreateExportOperation(const String& nameTemplate,
      Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result);
    TextResultOperation::Ptr CreateExportOperation(Playlist::Model::IndexSetPtr items,
      const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result);
  }
}
