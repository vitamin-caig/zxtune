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
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("FlacSettings", msg, 0, QApplication::UnicodeUTF8);
  }

  class FLACSettingsWidget : public UI::BackendSettingsWidget
                           , private Ui::FlacSettings
  {
  public:
    explicit FLACSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      connect(compressionValue, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));

      using namespace Parameters;
      IntegerValue::Bind(*compressionValue, *Options, ZXTune::Sound::Backends::Flac::COMPRESSION,
        ZXTune::Sound::Backends::Flac::COMPRESSION_DEFAULT);
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'f', 'l', 'a', 'c', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return Translate(QT_TRANSLATE_NOOP("FlacSettings", "Compression %1")).arg(compressionValue->value());
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}

namespace UI
{
  BackendSettingsWidget* CreateFLACSettingsWidget(QWidget& parent)
  {
    return new FLACSettingsWidget(parent);
  }
}
