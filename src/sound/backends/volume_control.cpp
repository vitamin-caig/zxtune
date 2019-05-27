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
#include "sound/backends/volume_control.h"
//common includes
#include <make_ptr.h>
//library includes
#include <l10n/api.h>

#define FILE_TAG B368C82C

namespace Sound
{
  class VolumeControlDelegate : public VolumeControl
  {
    static const L10n::TranslateFunctor translate;
  public:
    explicit VolumeControlDelegate(VolumeControl::Ptr delegate)
      : Delegate(delegate)
    {
    }

    Gain GetVolume() const override
    {
      if (const VolumeControl::Ptr delegate = Delegate.lock())
      {
        return delegate->GetVolume();
      }
      throw Error(THIS_LINE, translate("Failed to get volume in invalid state."));
    }

    void SetVolume(const Gain& volume) override
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

  //TODO: extract single translate functor
  const L10n::TranslateFunctor VolumeControlDelegate::translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
  VolumeControl::Ptr CreateVolumeControlDelegate(VolumeControl::Ptr delegate)
  {
    return MakePtr<VolumeControlDelegate>(delegate);
  }
}

#undef FILE_TAG
