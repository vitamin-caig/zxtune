/**
 *
 * @file
 *
 * @brief  Mixer implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/impl/mixer_core.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
// std includes
#include <algorithm>
#include <numeric>

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  template<unsigned Channels>
  class MixerImpl : public FixedChannelsMatrixMixer<Channels>
  {
    typedef FixedChannelsMatrixMixer<Channels> Base;

  public:
    MixerImpl()
    {
      const Gain::Type INVALID_GAIN_VALUE(Gain::Type::PRECISION, 1);
      const Gain INVALID_GAIN(INVALID_GAIN_VALUE, INVALID_GAIN_VALUE);
      std::fill(LastMatrix.begin(), LastMatrix.end(), INVALID_GAIN);
    }

    Sample ApplyData(const typename Base::InDataType& in) const override
    {
      return Core.Mix(in);
    }

    void SetMatrix(const typename Base::Matrix& data) override
    {
      if (std::any_of(data.begin(), data.end(), [](Gain gain) { return !gain.IsNormalized(); }))
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: gain is out of range."));
      }
      if (LastMatrix != data)
      {
        Core.SetMatrix(data);
        LastMatrix = data;
      }
    }

  private:
    MixerCore<Channels> Core;
    typename Base::Matrix LastMatrix;
  };
}  // namespace Sound

namespace Sound
{
  template<>
  OneChannelMatrixMixer::Ptr OneChannelMatrixMixer::Create()
  {
    return MakePtr<MixerImpl<1> >();
  }

  template<>
  TwoChannelsMatrixMixer::Ptr TwoChannelsMatrixMixer::Create()
  {
    return MakePtr<MixerImpl<2> >();
  }

  template<>
  ThreeChannelsMatrixMixer::Ptr ThreeChannelsMatrixMixer::Create()
  {
    return MakePtr<MixerImpl<3> >();
  }

  template<>
  FourChannelsMatrixMixer::Ptr FourChannelsMatrixMixer::Create()
  {
    return MakePtr<MixerImpl<4> >();
  }
}  // namespace Sound
