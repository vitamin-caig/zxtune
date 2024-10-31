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

#include <core/core_parameters.h>
#include <sound/sound_parameters.h>

#include <contract.h>

#include <utility>

namespace
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
      Interpolation = IntegerValue::Bind(*interpolationValue, *Options, ZXTune::Core::SID::INTERPOLATION,
                                         ZXTune::Core::SID::INTERPOLATION_DEFAULT);
      BooleanValue::Bind(*filterValue, *Options, ZXTune::Core::SID::FILTER, ZXTune::Core::SID::FILTER_DEFAULT);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const Parameters::ValueSnapshot blockInterpolation(*Interpolation);
        retranslateUi(this);
      }
      UI::SIDSettingsWidget::changeEvent(event);
    }

  private:
    const Parameters::Container::Ptr Options;
    Parameters::Value* Interpolation;
  };
}  // namespace
namespace UI
{
  SIDSettingsWidget::SIDSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SIDSettingsWidget* SIDSettingsWidget::Create(QWidget& parent)
  {
    return new SIDOptionsWidget(parent);
  }
}  // namespace UI
