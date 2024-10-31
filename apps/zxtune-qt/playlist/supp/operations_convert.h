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

#include "apps/zxtune-qt/playlist/supp/conversion.h"
#include "apps/zxtune-qt/playlist/supp/operations.h"

#include <sound/service.h>

#include <string_view.h>

namespace Playlist::Item
{
  // export
  class ConversionResultNotification : public Playlist::TextNotification
  {
  public:
    using Ptr = std::shared_ptr<ConversionResultNotification>;

    virtual void AddSucceed() = 0;
    virtual void AddFailedToOpen(StringView path) = 0;
    virtual void AddFailedToConvert(StringView path, const Error& err) = 0;
  };

  TextResultOperation::Ptr CreateSoundFormatConvertOperation(Playlist::Model::IndexSet::Ptr items, StringView type,
                                                             Sound::Service::Ptr service,
                                                             ConversionResultNotification::Ptr result);

  TextResultOperation::Ptr CreateExportOperation(StringView nameTemplate, Parameters::Accessor::Ptr params,
                                                 ConversionResultNotification::Ptr result);
  TextResultOperation::Ptr CreateExportOperation(Playlist::Model::IndexSet::Ptr items, StringView nameTemplate,
                                                 Parameters::Accessor::Ptr params,
                                                 ConversionResultNotification::Ptr result);

  // dispatcher over factories described above
  TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSet::Ptr items, const Conversion::Options& opts,
                                                  ConversionResultNotification::Ptr result);
  TextResultOperation::Ptr CreateConvertOperation(const Conversion::Options& opts,
                                                  ConversionResultNotification::Ptr result);
}  // namespace Playlist::Item
