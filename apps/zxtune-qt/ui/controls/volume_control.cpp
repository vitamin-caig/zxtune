/**
 *
 * @file
 *
 * @brief Volume control widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "volume_control.h"
#include "supp/playback_supp.h"
#include "ui/styles.h"
#include "volume_control.ui.h"
// common includes
#include <contract.h>
#include <error.h>
// std includes
#include <ctime>
#include <numeric>
#include <utility>

namespace
{
  class VolumeControlImpl
    : public VolumeControl
    , public Ui::VolumeControl
  {
  public:
    VolumeControlImpl(QWidget& parent, PlaybackSupport& supp)
      : ::VolumeControl(parent)
    {
      // setup self
      setupUi(this);
      setEnabled(false);
      Require(connect(volumeLevel, &QSlider::valueChanged, this, &VolumeControlImpl::SetLevel));

      Require(connect(&supp, &PlaybackSupport::OnStartModule, this, &VolumeControlImpl::StartPlayback));
      Require(connect(&supp, &PlaybackSupport::OnUpdateState, this, &VolumeControlImpl::UpdateState));
      Require(connect(&supp, &PlaybackSupport::OnStopModule, this, &VolumeControlImpl::StopPlayback));
      volumeLevel->setStyle(UI::GetStyle());
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::VolumeControl::changeEvent(event);
    }

  private:
    void StartPlayback(Sound::Backend::Ptr backend, Playlist::Item::Data::Ptr)
    {
      Controller = backend->GetVolumeControl();
      setEnabled(Controller != nullptr);
    }

    void UpdateState()
    {
      if (isVisible() && Controller && !volumeLevel->isSliderDown())
      {
        UpdateVolumeSlider();
      }
    }

    void StopPlayback()
    {
      Controller = Sound::VolumeControl::Ptr();
      setEnabled(false);
    }

    void SetLevel(int level)
    {
      if (Controller)
      {
        const Sound::Gain::Type vol = Sound::Gain::Type(level, volumeLevel->maximum());
        Controller->SetVolume(Sound::Gain(vol, vol));
      }
    }

    void UpdateVolumeSlider()
    {
      try
      {
        const Sound::Gain vol = Controller->GetVolume();
        const Sound::Gain::Type gain = std::max(vol.Left(), vol.Right());
        volumeLevel->setValue(static_cast<int>((gain * volumeLevel->maximum()).Round()));
      }
      catch (const Error&)
      {}
    }

  private:
    Sound::VolumeControl::Ptr Controller;
  };
}  // namespace

VolumeControl::VolumeControl(QWidget& parent)
  : QWidget(&parent)
{}

VolumeControl* VolumeControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new VolumeControlImpl(parent, supp);
}
