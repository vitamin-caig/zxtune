/**
* 
* @file
*
* @brief OGG settings widget implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "ogg_settings.h"
#include "ogg_settings.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//library includes
#include <sound/backends_parameters.h>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("OggSettings", msg, 0, QApplication::UnicodeUTF8);
  }

  class OGGSettingsWidget : public UI::BackendSettingsWidget
                          , private Ui::OggSettings
  {
  public:
    explicit OGGSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      connect(selectQuality, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(qualityValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectBitrate, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(bitrateValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));

      using namespace Parameters;
      ExclusiveValue::Bind(*selectQuality, *Options, ZXTune::Sound::Backends::Ogg::MODE,
        ZXTune::Sound::Backends::Ogg::MODE_QUALITY);
      IntegerValue::Bind(*qualityValue, *Options, ZXTune::Sound::Backends::Ogg::QUALITY,
        ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT);
      ExclusiveValue::Bind(*selectBitrate, *Options, ZXTune::Sound::Backends::Ogg::MODE,
        ZXTune::Sound::Backends::Ogg::MODE_ABR);
      IntegerValue::Bind(*bitrateValue, *Options, ZXTune::Sound::Backends::Ogg::BITRATE,
        ZXTune::Sound::Backends::Ogg::BITRATE_DEFAULT);
      //fixup
      if (!selectQuality->isChecked() && !selectBitrate->isChecked())
      {
        selectQuality->setChecked(true);
      }
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
        return Translate(QT_TRANSLATE_NOOP("OggSettings", "Quality %1")).arg(qualityValue->value());
      }
      else if (selectBitrate->isChecked())
      {
        return Translate(QT_TRANSLATE_NOOP("OggSettings", "Average bitrate %1 kbps")).arg(bitrateValue->value());
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
  BackendSettingsWidget* CreateOGGSettingsWidget(QWidget& parent)
  {
    return new OGGSettingsWidget(parent);
  }
}
