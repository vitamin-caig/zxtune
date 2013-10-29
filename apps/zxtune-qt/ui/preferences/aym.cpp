/**
* 
* @file
*
* @brief AYM settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym.h"
#include "aym.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

namespace
{
  static const uint64_t PRESETS[] =
  {
    1773400,
    1750000,
    2000000,
    1000000,
    3500000
  };

  class AYMOptionsWidget : public UI::AYMSettingsWidget
                         , public Ui::AYMOptions
  {
  public:
    explicit AYMOptionsWidget(QWidget& parent)
      : UI::AYMSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      connect(clockRateValue, SIGNAL(textChanged(const QString&)), SLOT(OnClockRateChanged(const QString&)));
      connect(clockRatePresets, SIGNAL(currentIndexChanged(int)), SLOT(OnClockRatePresetChanged(int)));

      using namespace Parameters;
      const IntegerTraits clockRate(ZXTune::Core::AYM::CLOCKRATE, ZXTune::Core::AYM::CLOCKRATE_DEFAULT, ZXTune::Core::AYM::CLOCKRATE_MIN, ZXTune::Core::AYM::CLOCKRATE_MAX);
      BigIntegerValue::Bind(*clockRateValue, *Options, clockRate);
      BooleanValue::Bind(*dutyCycleGroup, *Options, ZXTune::Core::AYM::DUTY_CYCLE_MASK, false, 0x1f);
      IntegerValue::Bind(*dutyCycleValue, *Options, ZXTune::Core::AYM::DUTY_CYCLE, 50);
      IntegerValue::Bind(*interpolationValue, *Options, ZXTune::Core::AYM::INTERPOLATION, 0);
    }

    virtual void OnClockRateChanged(const QString& val)
    {
      const qlonglong num = val.toLongLong();
      const uint64_t* const preset = std::find(PRESETS, boost::end(PRESETS), num);
      if (preset == boost::end(PRESETS))
      {
        clockRatePresets->setCurrentIndex(0);//custom
      }
      else
      {
        clockRatePresets->setCurrentIndex(1 + (preset - PRESETS));
      }
    }

    virtual void OnClockRatePresetChanged(int idx)
    {
      if (idx != 0)
      {
        Require(idx <= int(boost::size(PRESETS)));
        clockRateValue->setText(QString::number(PRESETS[idx - 1]));
      }
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        //do not change preset or smth
        const AutoBlockSignal block(*clockRatePresets);
        retranslateUi(this);
        //restore combobox value
        OnClockRateChanged(clockRateValue->text());
      }
      UI::AYMSettingsWidget::changeEvent(event);
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}
namespace UI
{
  AYMSettingsWidget::AYMSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  AYMSettingsWidget* AYMSettingsWidget::Create(QWidget& parent)
  {
    return new AYMOptionsWidget(parent);
  }
}
