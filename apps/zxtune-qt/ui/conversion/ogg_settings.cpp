/*
Abstract:
  OGG settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "ogg_settings.h"
#include "ogg_settings.ui.h"
//common includes
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>

namespace
{
  class OGGSettingsWidget : public UI::BackendSettingsWidget
                          , private Ui::OggSettings
  {
  public:
    explicit OGGSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
    {
      //setup self
      setupUi(this);

      connect(selectQuality, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(qualityValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectABR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(abrBitrate, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));

      selectQuality->setChecked(true);
      qualityValue->setValue(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT);
    }

    virtual void SetSettings(const Parameters::Accessor& params)
    {
      selectQuality->setChecked(true);
      Parameters::IntType val = 0;
      if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Ogg::ABR, val))
      {
        if (in_range<uint_t>(val, abrBitrate->minimum(), abrBitrate->maximum()))
        {
          selectABR->setChecked(true);
          abrBitrate->setValue(val);
        }
      }
      else if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY, val))
      {
        if (in_range<uint_t>(val, qualityValue->minimum(), qualityValue->maximum()))
        {
          qualityValue->setValue(val);
        }
      }
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      const Parameters::Container::Ptr result = Parameters::Container::Create();
      if (selectABR->isChecked())
      {
        const uint_t abr = abrBitrate->value();
        result->SetIntValue(Parameters::ZXTune::Sound::Backends::Ogg::ABR, abr);
      }
      else if (selectQuality->isChecked())
      {
        const uint_t quality = qualityValue->value();
        result->SetIntValue(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY, quality);
      }
      return result;
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'o', 'g', 'g', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      if (selectQuality->isChecked())
      {
        return QString("Quality %1").arg(qualityValue->value());
      }
      else if (selectABR->isChecked())
      {
        return QString("ABR %1 kbps").arg(abrBitrate->value());
      }
      else
      {
        return QString();
      }
    }
  };
}

namespace UI
{
  BackendSettingsWidget* CreateOGGSettingsWidget(QWidget& parent)
  {
    return new OGGSettingsWidget(parent);
  }
}
