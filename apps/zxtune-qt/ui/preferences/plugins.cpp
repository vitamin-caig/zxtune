/**
* 
* @file
*
* @brief Plugins settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "plugins.h"
#include "plugins.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <core/plugins_parameters.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/range/end.hpp>

namespace
{
  const char* TYPES[] =
  {
    "",//all
    "AY",
    "SID",
    "SPC",
    "NSF",
    "NSFe",
    "GBS",
    "SAP",
    "HES",
  };
  
  class PluginsOptionsWidget : public UI::PluginsSettingsWidget
                             , public Ui::PluginsOptions
  {
  public:
    explicit PluginsOptionsWidget(QWidget& parent)
      : UI::PluginsSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      using namespace Parameters;
      //raw scaner
      IntegerValue::Bind(*rawMinSizeValue, *Options, ZXTune::Core::Plugins::Raw::MIN_SIZE, ZXTune::Core::Plugins::Raw::MIN_SIZE_DEFAULT);
      BooleanValue::Bind(*rawPlainDoubleAnalysis, *Options, ZXTune::Core::Plugins::Raw::PLAIN_DOUBLE_ANALYSIS, false);
      
      //durations
      FillDurationTypes();
      Require(connect(durationType, SIGNAL(currentIndexChanged(int)), SLOT(OnDurationTypeChanged(int))));
      Require(connect(durationValue, SIGNAL(valueChanged(int)), SLOT(OnDurationValueChanged(int))));
      Require(connect(durationDefault, SIGNAL(clicked()), SLOT(OnDurationDefault())));
      OnDurationTypeChanged(0);
    }
    
    virtual void OnDurationTypeChanged(int row)
    {
      const String type(TYPES[row]);
      DurationParameter = Parameters::ZXTune::Core::Plugins::DEFAULT_DURATION(type);
      
      Parameters::IntType val = Parameters::ZXTune::Core::Plugins::DEFAULT_DURATION_DEFAULT;
      const bool forAll = type.empty();
      const bool hasValue = Options->FindValue(DurationParameter, val);
      
      if (!forAll && !hasValue)
      {
        Options->FindValue(Parameters::ZXTune::Core::Plugins::DEFAULT_DURATION(String()), val);
      }
      SetDurationValue(val);
    }
    
    virtual void OnDurationValueChanged(int duration)
    {
      Options->SetValue(DurationParameter, duration);
      SetDurationText(duration);
    }

    virtual void OnDurationDefault()
    {
      Options->RemoveValue(DurationParameter);
      OnDurationTypeChanged(durationType->currentIndex());
    }
    
    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::PluginsSettingsWidget::changeEvent(event);
    }
  private:
    void FillDurationTypes()
    {
      std::for_each(TYPES + 1, boost::end(TYPES), boost::bind(&PluginsOptionsWidget::AddDurationType, this, _1));
    }
    
    void AddDurationType(const char* type)
    {
      durationType->addItem(type);
    }
    
    void SetDurationValue(int val)
    {
      //disable OnDurationValueChanged call
      const AutoBlockSignal block(*durationValue);
      durationValue->setValue(val);
      SetDurationText(val);
    }

    void SetDurationText(int val)
    {
      const int minutes = val / 60;
      const int seconds = val % 60;
      durationLabel->setText(QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
    }
  private:
    const Parameters::Container::Ptr Options;
    Parameters::NameType DurationParameter;
  };
}

namespace UI
{
  PluginsSettingsWidget::PluginsSettingsWidget(QWidget &parent)
    : QWidget(&parent)
  {
  }

  PluginsSettingsWidget* PluginsSettingsWidget::Create(QWidget &parent)
  {
    return new PluginsOptionsWidget(parent);
  }
}
