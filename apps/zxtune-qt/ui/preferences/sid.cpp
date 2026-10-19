/**
 *
 * @file
 *
 * @brief SID settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/sid.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "sid.ui.h"

#include "core/parameters.h"
#include "sound/parameters.h"

#include "contract.h"

#include <utility>

namespace UI
{
  class SIDOptionsWidget
    : public UI::SIDSettingsWidget
    , public Ui::SIDOptions
  {
  public:
    explicit SIDOptionsWidget(QWidget& parent)
      : UI::SIDSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      using namespace Parameters;
      Interpolation = Value::Bind<ZXTune::Core::SID::Interpolation>(*interpolationValue, *Options);
      Value::Bind<ZXTune::Core::SID::Filter>(*filterValue, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
      }
      UI::SIDSettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
    Value* Interpolation;
  };
  SIDSettingsWidget::SIDSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SIDSettingsWidget* SIDSettingsWidget::Create(QWidget& parent)
  {
    return new SIDOptionsWidget(parent);
  }
}  // namespace UI
