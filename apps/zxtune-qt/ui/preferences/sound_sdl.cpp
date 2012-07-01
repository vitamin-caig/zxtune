/*
Abstract:
  SDL settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound_sdl.h"
#include "sound_sdl.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>

namespace
{
  const std::string THIS_MODULE("UI::Preferences::SDL");
}

namespace
{
  class SdlOptionsWidget : public UI::SdlSettingsWidget
                         , public Ui::SdlOptions
  {
  public:
    explicit SdlOptionsWidget(QWidget& parent)
      : UI::SdlSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      using namespace Parameters::ZXTune::Sound::Backends::SDL;
      Parameters::IntegerValue::Bind(*buffers, *Options, BUFFERS, BUFFERS_DEFAULT);
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      //TODO
      return Parameters::Container::Ptr();
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'s', 'd', 'l', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return nameGroup->title();
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}
namespace UI
{
  SdlSettingsWidget::SdlSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {
  }

  BackendSettingsWidget* SdlSettingsWidget::Create(QWidget& parent)
  {
    return new SdlOptionsWidget(parent);
  }
}
