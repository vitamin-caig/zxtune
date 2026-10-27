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

#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/options/strings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "playback_options.ui.h"

#include "core/parameters.h"
#include "module/attributes.h"
#include "parameters/identifiers_set.h"
#include "parameters/merged_accessor.h"
#include "sound/parameters.h"
#include "strings/split.h"

#include "contract.h"

#include <QtWidgets/QLayout>

#include <utility>
#include <vector>

namespace
{
  // Apply a method to each widget directly placed into the layout
  template<class... Args>
  void ForEachChild(QLayout& layout, void (QWidget::*method)(Args...), Args... args)
  {
    for (int idx = 0, count = layout.count(); idx < count; ++idx)
    {
      if (auto* widget = layout.itemAt(idx)->widget())
      {
        (widget->*method)(args...);
      }
    }
  }

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
      // Looping is the only always-applicable control; the rest are shown per advertised parameters
      ForEachChild(*horizontalLayout, &QWidget::setVisible, false);
      CommonOptions->setVisible(true);
      UpdateOptionTexts();

      // common
      UI::Value::Bind<Parameters::ZXTune::Sound::Looped>(*isLooped, *Params);
      // AYM
      UI::Value::Bind<Parameters::ZXTune::Core::AYM::Chip>(*isYM, *Params);
      UI::Value::Bind<Parameters::ZXTune::Core::AYM::Layout>(*aymLayout, *Params);
      // DAC
      UI::Value::Bind<Parameters::ZXTune::Core::DAC::Interpolation>(*isDACInterpolated, *Params);

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
        UpdateOptionTexts();
      }
      ::PlaybackOptions::changeEvent(event);
    }

  private:
    //! @brief Shared option vocabulary; also re-applied on language change
    void UpdateOptionTexts()
    {
      isDACInterpolated->setToolTip(UI::Options::Interpolation());
      UI::Options::Fill(*aymLayout, UI::Options::LayoutValues());
    }

    class MuteState
    {
    public:
      explicit MuteState(Ui::PlaybackOptions& self)
        : Control(self.MuteOptions)
        , Layout(self.MuteButtons)
      {}

      void Init(const Playlist::Item::Data& item)
      {
        if (const auto channelsNames = item.GetModuleProperties()->FindString(Module::ATTR_CHANNELS_NAMES))
        {
          Params = item.GetAdjustedParameters();
          using namespace Parameters::ZXTune::Core;
          State = Parameters::Get<ChannelsMask>(*Params);
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
            Parameters::Set<Parameters::ZXTune::Core::ChannelsMask>(*Params, State);
          }
          else
          {
            Parameters::Remove<Parameters::ZXTune::Core::ChannelsMask>(*Params);
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
      ModuleProperties = item->GetModuleProperties();
      // Show only the parameters the renderer actually reads
      const auto supported = Parameters::GetString(*ModuleProperties, Module::ATTR_SUPPORTED_PARAMETERS);
      const auto ids = Parameters::IdentifiersSet::Parse(supported);
      using namespace Parameters::ZXTune::Core;
      isYM->setVisible(ids.Contains(AYM::Chip::Key));
      aymLayout->setVisible(ids.Contains(AYM::Layout::Key));
      isDACInterpolated->setVisible(ids.Contains(DAC::Interpolation::Key));

      Muting.Init(*item);  // muting is only in adjusted

      setEnabled(true);
    }

    void UpdateState()
    {
      if (isVisible() && ModuleProperties)
      {
        // TODO: use walker?
        using namespace Parameters::ZXTune::Core;
        isYM->setEnabled(!Parameters::Find<AYM::Chip>(*ModuleProperties).has_value());
        aymLayout->setEnabled(!Parameters::Find<AYM::Layout>(*ModuleProperties).has_value());
        isDACInterpolated->setEnabled(!Parameters::Find<DAC::Interpolation>(*ModuleProperties).has_value());
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
