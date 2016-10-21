/**
* 
* @file
*
* @brief Volume control widget implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "volume_control.h"
#include "volume_control.ui.h"
#include "supp/playback_supp.h"
#include "ui/styles.h"
//common includes
#include <contract.h>
#include <error.h>
//std includes
#include <ctime>
#include <numeric>
#include <utility>

namespace
{
  class VolumeControlImpl : public VolumeControl
                          , public Ui::VolumeControl
  {
  public:
    VolumeControlImpl(QWidget& parent, PlaybackSupport& supp)
      : ::VolumeControl(parent)
    {
      //setup self
      setupUi(this);
      setEnabled(false);
      Require(connect(volumeLevel, SIGNAL(valueChanged(int)), SLOT(SetLevel(int))));

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(StartPlayback(Sound::Backend::Ptr))));
      Require(connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState())));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(StopPlayback())));
      volumeLevel->setStyle(UI::GetStyle());
    }

    void StartPlayback(Sound::Backend::Ptr backend) override
    {
      Controller = backend->GetVolumeControl();
      setEnabled(Controller != nullptr);
    }

    void UpdateState() override
    {
      if (isVisible() && Controller && !volumeLevel->isSliderDown())
      {
        UpdateVolumeSlider();
      }
    }

    void StopPlayback() override
    {
      Controller = Sound::VolumeControl::Ptr();
      setEnabled(false);
    }

    void SetLevel(int level) override
    {
      if (Controller)
      {
        const Sound::Gain::Type vol = Sound::Gain::Type(level, volumeLevel->maximum());
        Controller->SetVolume(Sound::Gain(vol, vol));
      }
    }

    //QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::VolumeControl::changeEvent(event);
    }
  private:
    void UpdateVolumeSlider()
    {
      try
      {
        const Sound::Gain vol = Controller->GetVolume();
        const Sound::Gain::Type gain = std::max(vol.Left(), vol.Right());
        volumeLevel->setValue(static_cast<int>((gain * volumeLevel->maximum()).Round()));
      }
      catch (const Error&)
      {
      }
    }
  private:
    Sound::VolumeControl::Ptr Controller;
  };
}

VolumeControl::VolumeControl(QWidget& parent) : QWidget(&parent)
{
}

VolumeControl* VolumeControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new VolumeControlImpl(parent, supp);
}
