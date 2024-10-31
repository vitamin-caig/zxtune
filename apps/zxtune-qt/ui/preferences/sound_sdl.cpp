/**
 *
 * @file
 *
 * @brief SDL settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/sound_sdl.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "sound_sdl.ui.h"

#include "parameters/container.h"
#include "parameters/identifier.h"
#include "sound/backends_parameters.h"

#include "string_view.h"

#include <QtCore/QEvent>
#include <QtWidgets/QGroupBox>

#include <memory>

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
