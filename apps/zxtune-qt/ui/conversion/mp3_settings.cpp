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
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <math/numeric.h>
#include <sound/backends_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("Mp3Settings", msg, 0, QApplication::UnicodeUTF8);
  }

  const Parameters::StringType CHANNEL_MODES[] =
  {
    Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT,
    Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_STEREO,
    Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_JOINTSTEREO,
    Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_MONO
  };

  class ChannelModeComboboxValue : public Parameters::Integer
  {
  public:
    explicit ChannelModeComboboxValue(Parameters::Container::Ptr ctr)
      : Ctr(ctr)
    {
    }

    virtual int Get() const
    {
      using namespace Parameters;
      Parameters::StringType val = ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT;
      Ctr->FindValue(ZXTune::Sound::Backends::Mp3::CHANNELS, val);
      const Parameters::StringType* const arrPos = std::find(CHANNEL_MODES, ArrayEnd(CHANNEL_MODES), val);
      return arrPos != ArrayEnd(CHANNEL_MODES)
        ? arrPos - CHANNEL_MODES
        : -1;
    }

    virtual void Set(int val)
    {
      if (Math::InRange<int>(val, 0, ArraySize(CHANNEL_MODES) - 1))
      {
        Ctr->SetValue(Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS, CHANNEL_MODES[val]);
      }
    }

    virtual void Reset()
    {
      Ctr->RemoveValue(Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS);
    }
  private:
    const Parameters::Container::Ptr Ctr;
  };

  class MP3SettingsWidget : public UI::BackendSettingsWidget
                          , private Ui::Mp3Settings
  {
  public:
    explicit MP3SettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      Require(connect(selectCBR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged())));
      Require(connect(selectABR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged())));
      Require(connect(bitrateValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged())));
      Require(connect(selectQuality, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged())));
      Require(connect(qualityValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged())));
      Require(connect(channelsMode, SIGNAL(currentIndexChanged(int)), SIGNAL(SettingsChanged())));

      using namespace Parameters;
      ExclusiveValue::Bind(*selectCBR, *Options, ZXTune::Sound::Backends::Mp3::MODE,
        ZXTune::Sound::Backends::Mp3::MODE_CBR);
      ExclusiveValue::Bind(*selectABR, *Options, ZXTune::Sound::Backends::Mp3::MODE,
        ZXTune::Sound::Backends::Mp3::MODE_ABR);
      ExclusiveValue::Bind(*selectQuality, *Options, ZXTune::Sound::Backends::Mp3::MODE,
        ZXTune::Sound::Backends::Mp3::MODE_VBR);
      IntegerValue::Bind(*bitrateValue, *Options, ZXTune::Sound::Backends::Mp3::BITRATE,
        ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT);
      IntegerValue::Bind(*qualityValue, *Options, ZXTune::Sound::Backends::Mp3::QUALITY,
        ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT);
      IntegerValue::Bind(*channelsMode, boost::make_shared<ChannelModeComboboxValue>(Options));
      //fixup
      if (!selectCBR->isChecked() && !selectABR->isChecked() && !selectQuality->isChecked())
      {
        selectCBR->setChecked(true);
      }
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      using namespace Parameters;
      const Container::Ptr result = Container::Create();
      CopyExistingValue<StringType>(*Options, *result, ZXTune::Sound::Backends::Mp3::MODE);
      CopyExistingValue<IntType>(*Options, *result, ZXTune::Sound::Backends::Mp3::BITRATE);
      CopyExistingValue<IntType>(*Options, *result, ZXTune::Sound::Backends::Mp3::QUALITY);
      return result;
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'m', 'p', '3', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      QString descr = GetBitrateDescription();
      if (0 != channelsMode->currentIndex())
      {
        descr += QLatin1String(", ");
        descr += channelsMode->currentText();
      }
      return descr;
    }
  private:
    QString GetBitrateDescription() const
    {
      if (selectCBR->isChecked() || selectABR->isChecked())
      {
        const int bitrate = bitrateValue->value();
        return selectCBR->isChecked()
          ? Translate(QT_TRANSLATE_NOOP("Mp3Settings", "%1 kbps")).arg(bitrate)
          : Translate(QT_TRANSLATE_NOOP("Mp3Settings", "~ %1 kbps")).arg(bitrate);
      }
      else if (selectQuality->isChecked())
      {
        return Translate(QT_TRANSLATE_NOOP("Mp3Settings", "Quality %1")).arg(qualityValue->value());
      }
      else
      {
        return QString();
      }
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}

namespace UI
{
  BackendSettingsWidget* CreateMP3SettingsWidget(QWidget& parent)
  {
    return new MP3SettingsWidget(parent);
  }
}
