/*
Abstract:
  Volume control widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "volume_control.h"
#include "volume_control_ui.h"
#include "volume_control_moc.h"
//common includes
#include <error.h>
//std includes
#include <ctime>
#include <numeric>

namespace
{
  class VolumeControlImpl : public VolumeControl
                          , public Ui::VolumeControl
  {
  public:
    explicit VolumeControlImpl(QWidget* parent)
      : LastUpdateTime(0)
    {
      setParent(parent);
      setupUi(this);
      this->setEnabled(false);
      this->connect(volumeLevel, SIGNAL(valueChanged(int)), SLOT(SetLevel(int)));
    }

    virtual void SetBackend(const ZXTune::Sound::Backend& backend)
    {
      Controller = backend.GetVolumeControl();
      this->setEnabled(Controller != 0);
    }

    virtual void UpdateState()
    {
      //slight optimization
      const std::time_t thisTime = std::time(0);
      if (thisTime == LastUpdateTime)
      {
        return;
      }
      LastUpdateTime = thisTime;
      if (Controller && !volumeLevel->isSliderDown())
      {
        ZXTune::Sound::MultiGain vol;
        //TODO: check result
        Controller->GetVolume(vol);
        const ZXTune::Sound::Gain gain = std::accumulate(vol.begin(), vol.end(), ZXTune::Sound::Gain(0), std::plus<ZXTune::Sound::Gain>()) / vol.size();
        volumeLevel->setValue(static_cast<int>(gain * volumeLevel->maximum()));
      }
    }

    virtual void SetLevel(int level)
    {
      if (Controller)
      {
        const ZXTune::Sound::Gain gain = ZXTune::Sound::Gain(level) / volumeLevel->maximum();
        const ZXTune::Sound::MultiGain vol = { {gain} };
        //TODO: check result
        Controller->SetVolume(vol);
      }
    }
  private:
    ZXTune::Sound::VolumeControl::Ptr Controller;
    std::time_t LastUpdateTime;
  };
}

VolumeControl* VolumeControl::Create(QWidget* parent)
{
  assert(parent);
  return new VolumeControlImpl(parent);
}
