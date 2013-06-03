/*
Abstract:
  Mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "mixer_core.h"
//common includes
#include <tools.h>
#include <error_tools.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
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
  class StreamMixer : public FixedChannelsMatrixStreamMixer<Channels>
  {
    typedef FixedChannelsMatrixStreamMixer<Channels> Base;
  public:
    StreamMixer()
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

  template<unsigned Channels>
  class Mixer : public FixedChannelsMatrixMixer<Channels>
  {
    typedef FixedChannelsMatrixMixer<Channels> Base;
  public:
    virtual Sample ApplyData(const typename Base::InDataType& in)
    {
      return Core.Mix(in);
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
    MixerCore<Channels> Core;
  };
}

namespace Sound
{
  template<>
  FixedChannelsMatrixStreamMixer<1>::Ptr FixedChannelsMatrixStreamMixer<1>::Create()
  {
    return boost::make_shared<StreamMixer<1> >();
  }

  template<>
  FixedChannelsMatrixStreamMixer<2>::Ptr FixedChannelsMatrixStreamMixer<2>::Create()
  {
    return boost::make_shared<StreamMixer<2> >();
  }

  template<>
  FixedChannelsMatrixStreamMixer<3>::Ptr FixedChannelsMatrixStreamMixer<3>::Create()
  {
    return boost::make_shared<StreamMixer<3> >();
  }

  template<>
  FixedChannelsMatrixStreamMixer<4>::Ptr FixedChannelsMatrixStreamMixer<4>::Create()
  {
    return boost::make_shared<StreamMixer<4> >();
  }

  template<>
  FixedChannelsMatrixMixer<1>::Ptr FixedChannelsMatrixMixer<1>::Create()
  {
    return boost::make_shared<Mixer<1> >();
  }

  template<>
  FixedChannelsMatrixMixer<2>::Ptr FixedChannelsMatrixMixer<2>::Create()
  {
    return boost::make_shared<Mixer<2> >();
  }

  template<>
  FixedChannelsMatrixMixer<3>::Ptr FixedChannelsMatrixMixer<3>::Create()
  {
    return boost::make_shared<Mixer<3> >();
  }

  template<>
  FixedChannelsMatrixMixer<4>::Ptr FixedChannelsMatrixMixer<4>::Create()
  {
    return boost::make_shared<Mixer<4> >();
  }
}
