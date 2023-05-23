/**
 *
 * @file
 *
 * @brief Playback options widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "playback_options.h"
#include "playback_options.ui.h"
#include "playlist/supp/capabilities.h"
#include "supp/playback_supp.h"
#include "ui/tools/parameters_helpers.h"
// common includes
#include <contract.h>
// library includes
#include <core/core_parameters.h>
#include <parameters/merged_accessor.h>
#include <sound/sound_parameters.h>
// std includes
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

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
                      SLOT(InitState(Sound::Backend::Ptr, Playlist::Item::Data::Ptr))));
      Require(connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState())));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
    }

    void InitState(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item) override
    {
      const Playlist::Item::Capabilities& caps = item->GetCapabilities();
      AYMOptions->setVisible(caps.IsAYM());
      DACOptions->setVisible(caps.IsDAC());
      SetEnabled(true);

      ModuleProperties = item->GetModuleProperties();
    }

    void UpdateState() override
    {
      if (isVisible() && ModuleProperties)
      {
        // TODO: use walker?
        Parameters::IntType val;
        isYM->setEnabled(!ModuleProperties->FindValue(Parameters::ZXTune::Core::AYM::TYPE, val));
        aymLayout->setEnabled(!ModuleProperties->FindValue(Parameters::ZXTune::Core::AYM::LAYOUT, val));
        isDACInterpolated->setEnabled(!ModuleProperties->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, val));
      }
    }

    void CloseState() override
    {
      SetEnabled(false);
      ModuleProperties = {};
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
