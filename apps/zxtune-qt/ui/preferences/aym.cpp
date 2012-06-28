/*
Abstract:
  AYM settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "aym.h"
#include "aym.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>

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
      BigIntegerValue::Bind(*clockRateValue, *Options, ZXTune::Core::AYM::CLOCKRATE, ZXTune::Core::AYM::CLOCKRATE_DEFAULT);
      BooleanValue::Bind(*dutyCycleGroup, *Options, ZXTune::Core::AYM::DUTY_CYCLE_MASK, false, 0x1f);
      IntegerValue::Bind(*dutyCycleValue, *Options, ZXTune::Core::AYM::DUTY_CYCLE, 50);
    }

    virtual void OnClockRateChanged(const QString& val)
    {
      const qlonglong num = val.toLongLong();
      const uint64_t* const preset = std::find(PRESETS, ArrayEnd(PRESETS), num);
      if (preset == ArrayEnd(PRESETS))
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
        Require(idx <= int(ArraySize(PRESETS)));
        clockRateValue->setText(QString::number(PRESETS[idx - 1]));
      }
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
