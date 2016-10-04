/**
*
* @file
*
* @brief  Volume control delegate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "volume_control.h"
//common includes
#include <make_ptr.h>
//library includes
#include <l10n/api.h>

#define FILE_TAG B368C82C

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");

  class VolumeControlDelegate : public VolumeControl
  {
  public:
    explicit VolumeControlDelegate(VolumeControl::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual Gain GetVolume() const
    {
      if (const VolumeControl::Ptr delegate = Delegate.lock())
      {
        return delegate->GetVolume();
      }
      throw Error(THIS_LINE, translate("Failed to get volume in invalid state."));
    }

    virtual void SetVolume(const Gain& volume)
    {
      if (const VolumeControl::Ptr delegate = Delegate.lock())
      {
        return delegate->SetVolume(volume);
      }
      throw Error(THIS_LINE, translate("Failed to set volume in invalid state."));
    }
  private:
    const std::weak_ptr<VolumeControl> Delegate;
  };
}

namespace Sound
{
  VolumeControl::Ptr CreateVolumeControlDelegate(VolumeControl::Ptr delegate)
  {
    return MakePtr<VolumeControlDelegate>(delegate);
  }
}
