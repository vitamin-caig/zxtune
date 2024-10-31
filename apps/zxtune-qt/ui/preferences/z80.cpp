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
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "z80.ui.h"

#include "core/core_parameters.h"

#include <utility>

namespace
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

      using namespace Parameters;
      const IntegerTraits clockRate(ZXTune::Core::Z80::CLOCKRATE, ZXTune::Core::Z80::CLOCKRATE_DEFAULT,
                                    ZXTune::Core::Z80::CLOCKRATE_MIN, ZXTune::Core::Z80::CLOCKRATE_MAX);
      BigIntegerValue::Bind(*clockRateValue, *Options, clockRate);
      IntegerValue::Bind(*intDurationValue, *Options, ZXTune::Core::Z80::INT_TICKS,
                         ZXTune::Core::Z80::INT_TICKS_DEFAULT);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::Z80SettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
  };
}  // namespace

namespace UI
{
  Z80SettingsWidget::Z80SettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  Z80SettingsWidget* Z80SettingsWidget::Create(QWidget& parent)
  {
    return new Z80OptionsWidget(parent);
  }
}  // namespace UI
