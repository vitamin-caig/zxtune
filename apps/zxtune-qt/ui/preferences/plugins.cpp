/*
Abstract:
  Plugins settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

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
