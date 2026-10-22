/**
 *
 * @file
 *
 * @brief Z80 settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/z80.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/options/strings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "z80.ui.h"

#include "core/parameters.h"

#include <utility>

namespace UI
{
  class Z80OptionsWidget
    : public UI::Z80SettingsWidget
    , public Ui::Z80Options
  {
  public:
    explicit Z80OptionsWidget(QWidget& parent)
      : UI::Z80SettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      clockRateLabel->setText(UI::Options::Clockrate());

      using namespace Parameters;
      Value::Bind<ZXTune::Core::Z80::Clockrate>(*clockRateValue, *Options);
      Value::Bind<ZXTune::Core::Z80::IntTicks>(*intDurationValue, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        clockRateLabel->setText(UI::Options::Clockrate());
      }
      UI::Z80SettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
  };
  Z80SettingsWidget::Z80SettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  Z80SettingsWidget* Z80SettingsWidget::Create(QWidget& parent)
  {
    return new Z80OptionsWidget(parent);
  }
}  // namespace UI
