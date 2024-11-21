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

#include "core/core_parameters.h"
#include "module/attributes.h"
#include "parameters/merged_accessor.h"
#include "sound/sound_parameters.h"
#include "strings/split.h"

#include "contract.h"

#include <utility>
#include <vector>

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
      , Muting(*this)
    {
      // setup self
      setupUi(this);
      AYMOptions->setVisible(false);
      DACOptions->setVisible(false);
      Muting.SetVisible(false);

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
    class MuteState
    {
    public:
      explicit MuteState(Ui::PlaybackOptions& self)
        : Control(self.MuteOptions)
        , Layout(self.MuteButtons)
      {}

      void SetVisible(bool visible)
      {
        Control->setVisible(visible);
      }

      void Init(const Playlist::Item::Data& item)
      {
        if (const auto channelsNames = item.GetModuleProperties()->FindString(Module::ATTR_CHANNELS_NAMES))
        {
          Params = item.GetAdjustedParameters();
          using namespace Parameters::ZXTune::Core;
          State = Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT);
          Control->setVisible(true);
          const auto names = Strings::Split(*channelsNames, '\n');
          for (std::size_t idx = 0, namesCount = names.size(), buttonsCount = Buttons.size();
               idx < std::max(namesCount, buttonsCount); ++idx)
          {
            auto& button = GetChannelControlButton(idx);
            const auto hasChannel = idx < namesCount;
            button.setVisible(hasChannel);
            if (hasChannel)
            {
              if (namesCount <= 8)
              {
                button.setText(ToQString(names[idx]));
                button.setToolTip({});
              }
              else
              {
                button.setText(QString::number(idx));
                button.setToolTip(ToQString(names[idx]));
              }
              button.setChecked(0 == (State & (1 << idx)));
            }
          }
        }
        else
        {
          Params = {};
          Control->setVisible(false);
        }
      }

    private:
      QToolButton& GetChannelControlButton(std::size_t idx)
      {
        Buttons.resize(std::max(idx + 1, Buttons.size()));
        auto*& res = Buttons[idx];
        if (!res)
        {
          res = new QToolButton(Control);
          res->setCheckable(true);
          res->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
          Layout->addWidget(res);
          Require(connect(res, &QToolButton::toggled, Control, [this, idx](bool enabled) { Apply(idx, enabled); }));
        }
        return *res;
      }

      void Apply(uint_t idx, bool enabled)
      {
        const auto mask = 1 << idx;
        State |= mask;
        if (enabled)
        {
          State ^= mask;
        }
        if (Params)
        {
          if (State)
          {
            Params->SetValue(Parameters::ZXTune::Core::CHANNELS_MASK, State);
          }
          else
          {
            Params->RemoveValue(Parameters::ZXTune::Core::CHANNELS_MASK);
          }
        }
      }

    public:
      QFrame*& Control;
      QHBoxLayout*& Layout;
      Parameters::Container::Ptr Params;
      Parameters::IntType State{};
      std::vector<QToolButton*> Buttons{16};
    };

    void InitState(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      const auto& caps = item->GetCapabilities();
      AYMOptions->setVisible(caps.IsAYM());
      DACOptions->setVisible(caps.IsDAC());

      ModuleProperties = item->GetModuleProperties();
      Muting.Init(*item);  // muting is only in adjusted

      setEnabled(true);
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
      setEnabled(false);
      ModuleProperties = {};
    }

  private:
    const Parameters::Container::Ptr Params;
    Parameters::Accessor::Ptr ModuleProperties;
    MuteState Muting;
  };
}  // namespace

PlaybackOptions::PlaybackOptions(QWidget& parent)
  : QWidget(&parent)
{}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
{
  return new PlaybackOptionsImpl(parent, supp, std::move(params));
}
