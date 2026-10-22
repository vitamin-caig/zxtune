/**
 *
 * @file
 *
 * @brief SAA settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/saa.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/options/strings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "saa.ui.h"

#include "core/parameters.h"
#include "sound/parameters.h"

#include "contract.h"

#include <utility>

namespace UI
{
  class SAAOptionsWidget
    : public UI::SAASettingsWidget
    , public Ui::SAAOptions
  {
  public:
    explicit SAAOptionsWidget(QWidget& parent)
      : UI::SAASettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      clockRateGroup->setTitle(UI::Options::Clockrate());
      interpolationGroup->setTitle(UI::Options::Interpolation());
      UI::Options::Fill(*interpolationValue, UI::Options::InterpolationValues());

      using namespace Parameters;
      Value::Bind<ZXTune::Core::SAA::Clockrate>(*clockRateValue, *Options);
      Interpolation = Value::Bind<ZXTune::Core::SAA::Interpolation>(*interpolationValue, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
        clockRateGroup->setTitle(UI::Options::Clockrate());
        interpolationGroup->setTitle(UI::Options::Interpolation());
        UI::Options::Fill(*interpolationValue, UI::Options::InterpolationValues());
      }
      UI::SAASettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
    Value* Interpolation;
  };
  SAASettingsWidget::SAASettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SAASettingsWidget* SAASettingsWidget::Create(QWidget& parent)
  {
    return new SAAOptionsWidget(parent);
  }
}  // namespace UI
