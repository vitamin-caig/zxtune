/*
Abstract:
  Simple interpolation filter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/filter_factory.h>
//boost includes
#include <boost/make_shared.hpp>
namespace
{
  using namespace ZXTune::Sound;

  class SimpleInterpolationFilter : public Converter
  {
  public:
    SimpleInterpolationFilter()
      : Delegate(Receiver::CreateStub())
      , Prev()
    {
    }

    virtual void ApplyData(const OutputSample& data)
    {
      OutputSample result;
      for (uint_t chan = 0; chan != result.size(); ++chan)
      {
        result[chan] = (uint_t(data[chan]) + Prev[chan]) / 2;
      }
      Prev = data;
      return Delegate->ApplyData(result);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(Receiver::Ptr delegate)
    {
      Prev = OutputSample();
      Delegate = delegate ? delegate : Receiver::CreateStub();
    }
  private:
    Receiver::Ptr Delegate;
    OutputSample Prev;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    SimpleInterpolationFilter::Ptr CreateSimpleInterpolationFilter()
    {
      return boost::make_shared<SimpleInterpolationFilter>();
    }
  }
}
