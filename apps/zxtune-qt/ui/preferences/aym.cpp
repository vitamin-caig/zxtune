/**
 *
 * @file
 *
 * @brief AYM settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/aym.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/options/strings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "aym.ui.h"

#include "core/parameters.h"
#include "sound/parameters.h"

#include "contract.h"

#include <utility>

namespace
{
  const uint_t PRESETS[] = {
      1773400, 1750000, 3500000, 2000000, 1000000,
  };

}  // namespace

namespace UI
{
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

      clockRateGroup->setTitle(UI::Options::Clockrate());
      interpolationGroup->setTitle(UI::Options::Interpolation());
      UI::Options::Fill(*interpolationValue, UI::Options::InterpolationValues());

      Require(connect(clockRateValue, &QLineEdit::textChanged, this, &AYMOptionsWidget::OnClockRateChanged));
      Require(connect(clockRatePresets, qOverload<int>(&QComboBox::currentIndexChanged), this,
                      &AYMOptionsWidget::OnClockRatePresetChanged));

      using namespace Parameters;
      Value::Bind<ZXTune::Core::AYM::Clockrate>(*clockRateValue, *Options);
      Value::Bind<ZXTune::Core::AYM::DutyCycleMask>(*dutyCycleGroup, *Options, 7);
      Value::Bind<ZXTune::Core::AYM::DutyCycle>(*dutyCycleValue, *Options);
      Interpolation = Value::Bind<ZXTune::Core::AYM::Interpolation>(*interpolationValue, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        // do not change preset or smth
        const AutoBlockSignal blockClockrate(*clockRatePresets);
        const ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
        clockRateGroup->setTitle(UI::Options::Clockrate());
        interpolationGroup->setTitle(UI::Options::Interpolation());
        UI::Options::Fill(*interpolationValue, UI::Options::InterpolationValues());
        // restore combobox value
        OnClockRateChanged(clockRateValue->text());
      }
      UI::AYMSettingsWidget::changeEvent(event);
    }

  private:
    void OnClockRateChanged(const QString& val)
    {
      const qlonglong num = val.toLongLong();
      const uint_t* const preset = std::find(PRESETS, std::end(PRESETS), num);
      if (preset == std::end(PRESETS))
      {
        clockRatePresets->setCurrentIndex(0);  // custom
      }
      else
      {
        clockRatePresets->setCurrentIndex(1 + (preset - PRESETS));
      }
    }

    void OnClockRatePresetChanged(int idx)
    {
      if (idx != 0)
      {
        Require(idx <= int(std::size(PRESETS)));
        clockRateValue->setText(QString::number(PRESETS[idx - 1]));
      }
    }

  private:
    const Parameters::Container::Ptr Options;
    Value* Interpolation;
  };
  AYMSettingsWidget::AYMSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  AYMSettingsWidget* AYMSettingsWidget::Create(QWidget& parent)
  {
    return new AYMOptionsWidget(parent);
  }
}  // namespace UI
