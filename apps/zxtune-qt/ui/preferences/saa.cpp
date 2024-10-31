/**
 *
 * @file
 *
 * @brief SAA settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/preferences/saa.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "saa.ui.h"
// common includes
#include <contract.h>
// library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>
// std includes
#include <utility>

namespace
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

      using namespace Parameters;
      const IntegerTraits clockRate(ZXTune::Core::SAA::CLOCKRATE, ZXTune::Core::SAA::CLOCKRATE_DEFAULT,
                                    ZXTune::Core::SAA::CLOCKRATE_MIN, ZXTune::Core::SAA::CLOCKRATE_MAX);
      BigIntegerValue::Bind(*clockRateValue, *Options, clockRate);
      Interpolation = IntegerValue::Bind(*interpolationValue, *Options, ZXTune::Core::SAA::INTERPOLATION,
                                         ZXTune::Core::SAA::INTERPOLATION_DEFAULT);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const Parameters::ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
      }
      UI::SAASettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
    Parameters::Value* Interpolation;
  };
}  // namespace
namespace UI
{
  SAASettingsWidget::SAASettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SAASettingsWidget* SAASettingsWidget::Create(QWidget& parent)
  {
    return new SAAOptionsWidget(parent);
  }
}  // namespace UI
