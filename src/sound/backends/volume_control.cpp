/*
Abstract:
  Volume control delegate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "volume_control.h"
//library includes
#include <l10n/api.h>
//boost includes
#include <boost/make_shared.hpp>

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
    const boost::weak_ptr<VolumeControl> Delegate;
  };
}

namespace Sound
{
  VolumeControl::Ptr CreateVolumeControlDelegate(VolumeControl::Ptr delegate)
  {
    return boost::make_shared<VolumeControlDelegate>(delegate);
  }
}
