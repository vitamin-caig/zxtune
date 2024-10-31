/**
 *
 * @file
 *
 * @brief  Volume control delegate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/volume_control.h"

#include "sound/backends/l10n.h"

#include <make_ptr.h>

namespace Sound
{
  class VolumeControlDelegate : public VolumeControl
  {
  public:
    explicit VolumeControlDelegate(const VolumeControl::Ptr& delegate)
      : Delegate(delegate)
    {}

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
}  // namespace Sound

namespace Sound
{
  VolumeControl::Ptr CreateVolumeControlDelegate(const VolumeControl::Ptr& delegate)
  {
    return MakePtr<VolumeControlDelegate>(delegate);
  }
}  // namespace Sound
