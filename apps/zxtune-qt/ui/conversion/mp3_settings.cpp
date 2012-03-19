/*
Abstract:
  MP3 settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mp3_settings.h"
#include "mp3_settings.ui.h"
//common includes
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>

namespace
{
  const uint_t BITRATES[] =
  {
    32,
    64,
    96,
    112,
    128,
    160,
    192,
    256,
    320
  };

  class MP3SettingsWidget : public UI::BackendSettingsWidget
                    , private Ui::Mp3Settings
  {
  public:
    explicit MP3SettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
    {
      //setup self
      setupUi(this);
      std::for_each(BITRATES, ArrayEnd(BITRATES), std::bind1st(std::mem_fun(&MP3SettingsWidget::AddBitrate), this));

      connect(selectDefault, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(selectCBR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(bitrateValue, SIGNAL(currentIndexChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectVBR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(vbrQuality, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectABR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(abrBitrate, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));

      selectDefault->setChecked(true);
    }

    virtual void SetSettings(const Parameters::Accessor& params)
    {
      selectDefault->setChecked(true);
      Parameters::IntType val = 0;
      if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE, val))
      {
        const uint_t* const tableBitrate = std::upper_bound(BITRATES, ArrayEnd(BITRATES), val);
        if (tableBitrate != ArrayEnd(BITRATES))
        {
          selectCBR->setChecked(true);
          const uint_t idx = tableBitrate - BITRATES;
          bitrateValue->setCurrentIndex(idx);
        }
      }
      else if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Mp3::VBR, val))
      {
        if (in_range<uint_t>(val, vbrQuality->minimum(), vbrQuality->maximum()))
        {
          selectVBR->setChecked(true);
          vbrQuality->setValue(val);
        }
      }
      else if (params.FindIntValue(Parameters::ZXTune::Sound::Backends::Mp3::ABR, val))
      {
        if (in_range<uint_t>(val, abrBitrate->minimum(), abrBitrate->maximum()))
        {
          selectABR->setChecked(true);
          abrBitrate->setValue(val);
        }
      }
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      const Parameters::Container::Ptr result = Parameters::Container::Create();
      if (selectCBR->isChecked())
      {
        const uint_t brate = BITRATES[bitrateValue->currentIndex()];
        result->SetIntValue(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE, brate);
      }
      else if (selectVBR->isChecked())
      {
        const uint_t vbr = vbrQuality->value();
        result->SetIntValue(Parameters::ZXTune::Sound::Backends::Mp3::VBR, vbr);
      }
      else if (selectABR->isChecked())
      {
        const uint_t abr = abrBitrate->value();
        result->SetIntValue(Parameters::ZXTune::Sound::Backends::Mp3::ABR, abr);
      }
      return result;
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'m', 'p', '3', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      if (selectDefault->isChecked())
      {
        return selectDefault->text();
      }
      else if (selectCBR->isChecked())
      {
        return bitrateValue->currentText();
      }
      else if (selectVBR->isChecked())
      {
        return QString("VBR %1").arg(vbrQuality->value());
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
  private:
    void AddBitrate(uint_t brate) const
    {
      bitrateValue->addItem(QString("%1 kbps").arg(brate));
    }
  };
}

namespace UI
{
  BackendSettingsWidget* CreateMP3SettingsWidget(QWidget& parent)
  {
    return new MP3SettingsWidget(parent);
  }
}
