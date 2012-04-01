/*
Abstract:
  FLAC settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "flac_settings.h"
#include "flac_settings.ui.h"
//common includes
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>

namespace
{
  class FLACSettingsWidget : public UI::BackendSettingsWidget
                           , private Ui::FlacSettings
  {
  public:
    explicit FLACSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
    {
      //setup self
      setupUi(this);

      connect(compressionValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));
    }

    virtual void SetSettings(const Parameters::Accessor& params)
    {
      Parameters::IntType val = 0;
      if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Flac::COMPRESSION, val))
      {
        if (in_range<uint_t>(val, compressionValue->minimum(), compressionValue->maximum()))
        {
          compressionValue->setValue(val);
        }
      }
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      const Parameters::Container::Ptr result = Parameters::Container::Create();
      const uint_t compression = compressionValue->value();
      result->SetIntValue(Parameters::ZXTune::Sound::Backends::Flac::COMPRESSION, compression);
      return result;
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'f', 'l', 'a', 'c', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return QString("Compression %1").arg(compressionValue->value());
    }
  };
}

namespace UI
{
  BackendSettingsWidget* CreateFLACSettingsWidget(QWidget& parent)
  {
    return new FLACSettingsWidget(parent);
  }
}
