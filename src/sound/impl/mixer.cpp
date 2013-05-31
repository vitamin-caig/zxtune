/*
Abstract:
  Mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <tools.h>
#include <error_tools.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
#include <sound/mixer_core.h>
//std includes
#include <algorithm>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG 278565B1

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  template<unsigned Channels>
  class FastMixer : public FixedChannelsMatrixMixer<Channels>
  {
    typedef FixedChannelsMatrixMixer<Channels> Base;
  public:
    FastMixer()
      : Endpoint(Receiver::CreateStub())
    {
    }

    virtual void ApplyData(const typename Base::InDataType& in)
    {
      const Sample out = Core.Mix(in);
      return Endpoint->ApplyData(out);
    }
    
    virtual void Flush()
    {
      Endpoint->Flush();
    }
    
    virtual void SetTarget(Receiver::Ptr rcv)
    {
      Endpoint = rcv ? rcv : Receiver::CreateStub();
    }
    
    virtual void SetMatrix(const typename Base::Matrix& data)
    {
      const typename Base::Matrix::const_iterator it = std::find_if(data.begin(), data.end(), std::not1(std::mem_fun_ref(&Gain::IsNormalized)));
      if (it != data.end())
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: gain is out of range."));
      }
      Core.SetMatrix(data);
    }
  private:
    Receiver::Ptr Endpoint;
    MixerCore<Channels> Core;
  };
}

namespace Sound
{
  template<>
  FixedChannelsMatrixMixer<1>::Ptr FixedChannelsMatrixMixer<1>::Create()
  {
    return boost::make_shared<FastMixer<1> >();
  }

  template<>
  FixedChannelsMatrixMixer<2>::Ptr FixedChannelsMatrixMixer<2>::Create()
  {
    return boost::make_shared<FastMixer<2> >();
  }

  template<>
  FixedChannelsMatrixMixer<3>::Ptr FixedChannelsMatrixMixer<3>::Create()
  {
    return boost::make_shared<FastMixer<3> >();
  }

  template<>
  FixedChannelsMatrixMixer<4>::Ptr FixedChannelsMatrixMixer<4>::Create()
  {
    return boost::make_shared<FastMixer<4> >();
  }
}
