/**
* 
* @file
*
* @brief SAA settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "saa.h"
#include "saa.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>

namespace
{
  class SAAOptionsWidget : public UI::SAASettingsWidget
                         , public Ui::SAAOptions
  {
  public:
    explicit SAAOptionsWidget(QWidget& parent)
      : UI::SAASettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      using namespace Parameters;
      const IntegerTraits clockRate(ZXTune::Core::SAA::CLOCKRATE, ZXTune::Core::SAA::CLOCKRATE_DEFAULT, ZXTune::Core::SAA::CLOCKRATE_MIN, ZXTune::Core::SAA::CLOCKRATE_MAX);
      BigIntegerValue::Bind(*clockRateValue, *Options, clockRate);
      IntegerValue::Bind(*interpolationValue, *Options, ZXTune::Core::SAA::INTERPOLATION, 0);
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::SAASettingsWidget::changeEvent(event);
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}
namespace UI
{
  SAASettingsWidget::SAASettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  SAASettingsWidget* SAASettingsWidget::Create(QWidget& parent)
  {
    return new SAAOptionsWidget(parent);
  }
}
