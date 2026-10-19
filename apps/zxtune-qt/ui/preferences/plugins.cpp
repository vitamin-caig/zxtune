/**
 *
 * @file
 *
 * @brief Plugins settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/plugins.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "plugins.ui.h"

#include "core/plugins/parameters.h"

#include "contract.h"

#include <utility>

namespace UI
{
  class PluginsOptionsWidget
    : public UI::PluginsSettingsWidget
    , public Ui::PluginsOptions
  {
  public:
    explicit PluginsOptionsWidget(QWidget& parent)
      : UI::PluginsSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      using namespace Parameters;
      // raw scaner
      Value::Bind<ZXTune::Core::Plugins::Raw::MinSize>(*rawMinSizeValue, *Options);
      Value::Bind<ZXTune::Core::Plugins::Raw::PlainDoubleAnalysis>(*rawPlainDoubleAnalysis, *Options);

      // players
      Value::Bind<ZXTune::Core::Plugins::DefaultDuration>(*defaultDurationValue, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
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
  PluginsSettingsWidget::PluginsSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  PluginsSettingsWidget* PluginsSettingsWidget::Create(QWidget& parent)
  {
    return new PluginsOptionsWidget(parent);
  }
}  // namespace UI
