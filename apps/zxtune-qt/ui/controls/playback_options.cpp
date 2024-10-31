/**
 *
 * @file
 *
 * @brief Playback options widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/controls/playback_options.h"

#include "apps/zxtune-qt/playlist/supp/capabilities.h"
#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "playback_options.ui.h"

#include <core/core_parameters.h>
#include <parameters/merged_accessor.h>
#include <sound/sound_parameters.h>

#include <contract.h>

#include <utility>

namespace
{
  class PlaybackOptionsImpl
    : public PlaybackOptions
    , private Ui::PlaybackOptions
  {
  public:
    PlaybackOptionsImpl(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
      : ::PlaybackOptions(parent)
      , Params(std::move(params))
    {
      // setup self
      setupUi(this);
      AYMOptions->setVisible(false);
      DACOptions->setVisible(false);

      // common
      Parameters::BooleanValue::Bind(*isLooped, *Params, Parameters::ZXTune::Sound::LOOPED, false);
      // AYM
      Parameters::BooleanValue::Bind(*isYM, *Params, Parameters::ZXTune::Core::AYM::TYPE, false);
      Parameters::IntegerValue::Bind(*aymLayout, *Params, Parameters::ZXTune::Core::AYM::LAYOUT,
                                     Parameters::ZXTune::Core::AYM::LAYOUT_DEFAULT);
      // DAC
      Parameters::BooleanValue::Bind(*isDACInterpolated, *Params, Parameters::ZXTune::Core::DAC::INTERPOLATION,
                                     Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT);

      Require(connect(&supp, &PlaybackSupport::OnStartModule, this, &PlaybackOptionsImpl::InitState));
      Require(connect(&supp, &PlaybackSupport::OnUpdateState, this, &PlaybackOptionsImpl::UpdateState));
      Require(connect(&supp, &PlaybackSupport::OnStopModule, this, &PlaybackOptionsImpl::CloseState));
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::PlaybackOptions::changeEvent(event);
    }

  private:
    void InitState(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      const Playlist::Item::Capabilities& caps = item->GetCapabilities();
      AYMOptions->setVisible(caps.IsAYM());
      DACOptions->setVisible(caps.IsDAC());
      SetEnabled(true);

      ModuleProperties = item->GetModuleProperties();
    }

    void UpdateState()
    {
      if (isVisible() && ModuleProperties)
      {
        // TODO: use walker?
        using namespace Parameters::ZXTune::Core;
        isYM->setEnabled(!ModuleProperties->FindInteger(AYM::TYPE).has_value());
        aymLayout->setEnabled(!ModuleProperties->FindInteger(AYM::LAYOUT).has_value());
        isDACInterpolated->setEnabled(!ModuleProperties->FindInteger(DAC::INTERPOLATION).has_value());
      }
    }

    void CloseState()
    {
      SetEnabled(false);
      ModuleProperties = {};
    }

    void SetEnabled(bool enabled)
    {
      AYMOptions->setEnabled(enabled);
      DACOptions->setEnabled(enabled);
    }

  private:
    const Parameters::Container::Ptr Params;
    Parameters::Accessor::Ptr ModuleProperties;
  };
}  // namespace

PlaybackOptions::PlaybackOptions(QWidget& parent)
  : QWidget(&parent)
{}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
{
  return new PlaybackOptionsImpl(parent, supp, std::move(params));
}
