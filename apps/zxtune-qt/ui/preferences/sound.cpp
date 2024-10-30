/**
 *
 * @file
 *
 * @brief Sound settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound.h"
#include "sound.ui.h"
#include "sound_alsa.h"
#include "sound_dsound.h"
#include "sound_oss.h"
#include "sound_sdl.h"
#include "sound_win32.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/service.h>
#include <sound/sound_parameters.h>
#include <strings/map.h>
// std includes
#include <utility>

namespace
{
  const uint_t FREQUENCES[] = {8000, 12000, 16000, 22000, 24000, 32000, 44100, 48000};

  auto GetSystemBackends(Parameters::Accessor::Ptr params)
  {
    return Sound::CreateSystemService(std::move(params))->GetAvailableBackends();
  }

  class SoundOptionsWidget
    : public UI::SoundSettingsWidget
    , public UI::Ui_SoundSettingsWidget
  {
  public:
    explicit SoundOptionsWidget(QWidget& parent)
      : UI::SoundSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Backends(GetSystemBackends(Options))
    {
      // setup self
      setupUi(this);

      FillFrequences();
      FillBackends();

      using namespace Parameters;
      using namespace ZXTune::Sound;
      const auto freq = GetInteger(*Options, FREQUENCY, FREQUENCY_DEFAULT);
      SetFrequency(freq);
      Require(connect(soundFrequencyValue, qOverload<int>(&QComboBox::currentIndexChanged), this,
                      &SoundOptionsWidget::ChangeSoundFrequency));
      IntegerValue::Bind(*silenceLimitValue, *Options, SILENCE_LIMIT, SILENCE_LIMIT_DEFAULT);
      IntegerValue::Bind(*loopsCountLimitValue, *Options, LOOP_LIMIT, 0);
      IntegerValue::Bind(*fadeinValue, *Options, FADEIN, FADEIN_DEFAULT);
      IntegerValue::Bind(*fadeoutValue, *Options, FADEOUT, FADEOUT_DEFAULT);
      IntegerValue::Bind(*preampValue, *Options, GAIN, GAIN_DEFAULT);

      Require(connect(backendsList, &QListWidget::currentRowChanged, this, &SoundOptionsWidget::SelectBackend));
      Require(connect(moveUp, &QToolButton::released, this, &SoundOptionsWidget::MoveBackendUp));
      Require(connect(moveDown, &QToolButton::released, this, &SoundOptionsWidget::MoveBackendDown));
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::SoundSettingsWidget::changeEvent(event);
    }

  private:
    void FillFrequences()
    {
      for (const auto freq : FREQUENCES)
      {
        soundFrequencyValue->addItem(QString::number(freq));
      }
    }

    void FillBackends()
    {
      for (const auto& id : Backends)
      {
        backendsList->addItem(ToQString(id));
      }
      AddPage(&UI::AlsaSettingsWidget::Create);
      AddPage(&UI::DirectSoundSettingsWidget::Create);
      AddPage(&UI::OssSettingsWidget::Create);
      AddPage(&UI::SdlSettingsWidget::Create);
      AddPage(&UI::Win32SettingsWidget::Create);
    }

    void AddPage(UI::BackendSettingsWidget* (*factory)(QWidget&))
    {
      std::unique_ptr<UI::BackendSettingsWidget> wid(factory(*backendGroupBox));
      const auto id = wid->GetBackendId();
      if (Backends.end() != std::find(Backends.begin(), Backends.end(), id))
      {
        wid->hide();
        backendGroupLayout->addWidget(wid.get());
        SetupPages[id] = wid.release();
      }
    }

    void SetFrequency(uint_t val)
    {
      const uint_t* const frq = std::find(FREQUENCES, std::end(FREQUENCES), val);
      if (frq != std::end(FREQUENCES))
      {
        soundFrequencyValue->setCurrentIndex(frq - FREQUENCES);
      }
    }

    void ChangeSoundFrequency(int idx)
    {
      const qlonglong val = FREQUENCES[idx];
      Options->SetValue(Parameters::ZXTune::Sound::FREQUENCY, val);
    }

    void SelectBackend(int idx)
    {
      const auto& id = Backends[idx];
      for (const auto& page : SetupPages)
      {
        page.second->setVisible(page.first == id);
      }
      settingsHint->setVisible(0 == SetupPages.count(id));
    }

    void MoveBackendUp()
    {
      if (const int row = backendsList->currentRow())
      {
        SwapItems(row, row - 1);
        backendsList->setCurrentRow(row - 1);
      }
    }

    void MoveBackendDown()
    {
      const int row = backendsList->currentRow();
      if (Math::InRange(row, 0, int(Backends.size() - 2)))
      {
        SwapItems(row, row + 1);
        backendsList->setCurrentRow(row + 1);
      }
    }

    void SwapItems(int lh, int rh)
    {
      QListWidgetItem* const first = backendsList->item(lh);
      QListWidgetItem* const second = backendsList->item(rh);
      if (first && second)
      {
        const QString firstText = first->text();
        const QString secondText = second->text();
        auto& firstId = Backends[lh];
        auto& secondId = Backends[rh];
        assert(FromQString(firstText) == firstId);
        assert(FromQString(secondText) == secondId);
        first->setText(secondText);
        second->setText(firstText);
        std::swap(firstId, secondId);
        SaveBackendsOrder();
      }
    }

    void SaveBackendsOrder()
    {
      String value;
      for (const auto& id : Backends)
      {
        if (!value.empty())
        {
          value += ';';
        }
        value.append(id);
      }
      Options->SetValue(Parameters::ZXTune::Sound::Backends::ORDER, value);
    }

  private:
    const Parameters::Container::Ptr Options;
    std::vector<Sound::BackendId> Backends;
    Strings::ValueMap<QWidget*> SetupPages;
  };
}  // namespace
namespace UI
{
  SoundSettingsWidget::SoundSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SoundSettingsWidget* SoundSettingsWidget::Create(QWidget& parent)
  {
    return new SoundOptionsWidget(parent);
  }
}  // namespace UI
