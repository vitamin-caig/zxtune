/**
 *
 * @file
 *
 * @brief SDL settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/preferences/sound_sdl.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "sound_sdl.ui.h"
// common includes
#include <contract.h>
// library includes
#include <sound/backends_parameters.h>
// std includes
#include <utility>

namespace
{
  class SdlOptionsWidget
    : public UI::SdlSettingsWidget
    , public Ui::SdlOptions
  {
  public:
    explicit SdlOptionsWidget(QWidget& parent)
      : UI::SdlSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      using namespace Parameters::ZXTune::Sound::Backends::Sdl;
      Parameters::IntegerValue::Bind(*buffers, *Options, BUFFERS, BUFFERS_DEFAULT);
    }

    StringView GetBackendId() const override
    {
      return "sdl"sv;
    }

    QString GetDescription() const override
    {
      return nameGroup->title();
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::SdlSettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
  };
}  // namespace
namespace UI
{
  SdlSettingsWidget::SdlSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {}

  BackendSettingsWidget* SdlSettingsWidget::Create(QWidget& parent)
  {
    return new SdlOptionsWidget(parent);
  }
}  // namespace UI
