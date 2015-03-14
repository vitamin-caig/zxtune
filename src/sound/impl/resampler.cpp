/**
*
* @file
*
* @brief  Resampler implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
//library includes
#include <math/fixedpoint.h>
#include <sound/chunk_builder.h>
#include <sound/resampler.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Sound
{
  typedef Math::FixedPoint<int_t, 256> FixedStep;

  inline Sample::WideType Interpolate(Sample::WideType prev, FixedStep ratio, Sample::WideType next)
  {
    const Sample::WideType delta = next - prev;
    return prev + (ratio * delta).Integer();
  }
  
  inline Sample Interpolate(Sample prev, FixedStep ratio, Sample next)
  {
    if (0 == ratio.Raw())
    {
      return prev;
    }
    else
    {
      return Sample(Sound::Interpolate(prev.Left(), ratio, next.Left()), Sound::Interpolate(prev.Right(), ratio, next.Right()));
    }
  }
  
  class Upsampler : public Receiver
  {
  public:
    Upsampler(uint_t freqIn, uint_t freqOut, Receiver::Ptr delegate)
      : Delegate(delegate)
      , Step(freqIn, freqOut)
    {
      Require(freqIn < freqOut);
    }
    
    virtual void ApplyData(const Chunk::Ptr& data)
    {
      ChunkBuilder builder;
      builder.Reserve(1 + (FixedStep(data->size()) / Step).Round());
      Chunk::const_iterator it = data->begin(), lim = data->end();
      if (0 == Position.Raw())
      {
        Prev = *it;
        if (++it == lim)
        {
          return;
        }
      }
      Sample next = *it;
      for (; ; Position += Step)
      {
        if (Position.Raw() >= Position.PRECISION)
        {
          Position -= 1;
          Prev = next;
          if (++it == lim)
          {
            break;
          }
          next = *it;
        }
        builder.Add(Interpolate(Prev, Position, next));
      }
      Delegate->ApplyData(builder.GetResult());
    }
    
    virtual void Flush()
    {
      Delegate->Flush();
    }
  private:
    const Receiver::Ptr Delegate;
    const FixedStep Step;
    FixedStep Position;
    Sample Prev;
  };
  
  class Downsampler : public Receiver
  {
  public:
    Downsampler(uint_t freqIn, uint_t freqOut, Receiver::Ptr delegate)
      : Delegate(delegate)
      , Step(freqOut, freqIn)
    {
      Require(freqIn > freqOut);
    }

    virtual void ApplyData(const Chunk::Ptr& data)
    {
      ChunkBuilder builder;
      builder.Reserve(1 + (Step * data->size()).Round());
      Chunk::const_iterator it = data->begin(), lim = data->end();
      if (0 == Position.Raw())
      {
        Prev = *it;
        ++it;
      }
      for (; it != lim; Position += Step, ++it)
      {
        const Sample next = *it;
        if (Position.Raw() >= Position.PRECISION)
        {
          Position -= 1;
          builder.Add(Interpolate(Prev, Position, next));
        }
        Prev = next;
      }
      Delegate->ApplyData(builder.GetResult());
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }
  private:
    const Receiver::Ptr Delegate;
    const FixedStep Step;
    FixedStep Position;
    Sample Prev;
  };

  Receiver::Ptr CreateResampler(uint_t inFreq, uint_t outFreq, Receiver::Ptr delegate)
  {
    if (inFreq == outFreq)
    {
      return delegate;
    }
    else if (inFreq < outFreq)
    {
      return boost::make_shared<Upsampler>(inFreq, outFreq, delegate);
    }
    else
    {
      return boost::make_shared<Downsampler>(inFreq, outFreq, delegate);
    }
  }
}
