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
//library includes
#include <core/plugins_parameters.h>

namespace
{
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
    const Parameters::Container::Ptr Options;
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
