/**
 *
 * @file
 *
 * @brief AYM settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "aym.h"
#include "aym.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>
// std includes
#include <utility>
// boost includes
#include <boost/range/size.hpp>

namespace
{
  const uint64_t PRESETS[] = {
      1773400, 1750000, 3500000, 2000000, 1000000,
  };

  class AYMOptionsWidget
    : public UI::AYMSettingsWidget
    , public Ui::AYMOptions
  {
  public:
    explicit AYMOptionsWidget(QWidget& parent)
      : UI::AYMSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      Require(connect(clockRateValue, SIGNAL(textChanged(const QString&)), SLOT(OnClockRateChanged(const QString&))));
      Require(connect(clockRatePresets, SIGNAL(currentIndexChanged(int)), SLOT(OnClockRatePresetChanged(int))));

      using namespace Parameters;
      const IntegerTraits clockRate(ZXTune::Core::AYM::CLOCKRATE, ZXTune::Core::AYM::CLOCKRATE_DEFAULT,
                                    ZXTune::Core::AYM::CLOCKRATE_MIN, ZXTune::Core::AYM::CLOCKRATE_MAX);
      BigIntegerValue::Bind(*clockRateValue, *Options, clockRate);
      BooleanValue::Bind(*dutyCycleGroup, *Options, ZXTune::Core::AYM::DUTY_CYCLE_MASK, false, 7);
      IntegerValue::Bind(*dutyCycleValue, *Options, ZXTune::Core::AYM::DUTY_CYCLE,
                         ZXTune::Core::AYM::DUTY_CYCLE_DEFAULT);
      Interpolation = IntegerValue::Bind(*interpolationValue, *Options, ZXTune::Core::AYM::INTERPOLATION,
                                         ZXTune::Core::AYM::INTERPOLATION_DEFAULT);
    }

    void OnClockRateChanged(const QString& val) override
    {
      const qlonglong num = val.toLongLong();
      const uint64_t* const preset = std::find(PRESETS, std::end(PRESETS), num);
      if (preset == std::end(PRESETS))
      {
        clockRatePresets->setCurrentIndex(0);  // custom
      }
      else
      {
        clockRatePresets->setCurrentIndex(1 + (preset - PRESETS));
      }
    }

    void OnClockRatePresetChanged(int idx) override
    {
      if (idx != 0)
      {
        Require(idx <= int(boost::size(PRESETS)));
        clockRateValue->setText(QString::number(PRESETS[idx - 1]));
      }
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        // do not change preset or smth
        const AutoBlockSignal blockClockrate(*clockRatePresets);
        const Parameters::ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
        // restore combobox value
        OnClockRateChanged(clockRateValue->text());
      }
      UI::AYMSettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
    Parameters::Value* Interpolation;
  };
}  // namespace
namespace UI
{
  AYMSettingsWidget::AYMSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  AYMSettingsWidget* AYMSettingsWidget::Create(QWidget& parent)
  {
    return new AYMOptionsWidget(parent);
  }
}  // namespace UI
