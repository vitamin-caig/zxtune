/*
Abstract:
  Factory of receivers pair

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/receiver.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Sound
{
  class DoubleMixer
  {
  public:
    typedef boost::shared_ptr<DoubleMixer> Ptr;

    void Store(Chunk::Ptr in)
    {
      Buffer = in;
    }

    Chunk::Ptr Mix(Chunk::Ptr in)
    {
      assert(in->size() == Buffer->size());
      std::transform(Buffer->begin(), Buffer->end(), in->begin(), Buffer->begin(), &MixAll);
      return Buffer;
    }
  private:
    static inline Sample::Type MixChannel(Sample::Type lh, Sample::Type rh)
    {
      return (Sample::WideType(lh) + rh) / 2;
    }

    static inline Sample MixAll(Sample lh, Sample rh)
    {
      return Sample(MixChannel(lh.Left(), rh.Left()), MixChannel(lh.Right(), rh.Right()));
    }
  private:
    Chunk::Ptr Buffer;
  };

  class FirstReceiver : public Receiver
  {
  public:
    explicit FirstReceiver(DoubleMixer::Ptr mixer)
      : Mixer(mixer)
    {
    }

    virtual void ApplyData(const Chunk::Ptr& data)
    {
      Mixer->Store(data);
    }

    virtual void Flush()
    {
    }
  private:
    const DoubleMixer::Ptr Mixer;
  };

  class SecondReceiver : public Receiver
  {
  public:
    SecondReceiver(DoubleMixer::Ptr mixer, Receiver::Ptr delegate)
      : Mixer(mixer)
      , Delegate(delegate)
    {
    }

    virtual void ApplyData(const Chunk::Ptr& data)
    {
      Delegate->ApplyData(Mixer->Mix(data));
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }
  private:
    const DoubleMixer::Ptr Mixer;
    const Receiver::Ptr Delegate;
  };
}

namespace Sound
{
  std::pair<Receiver::Ptr, Receiver::Ptr> CreateReceiversPair(Sound::Receiver::Ptr target)
  {
    const DoubleMixer::Ptr mixer = boost::make_shared<DoubleMixer>();
    return std::make_pair(boost::make_shared<FirstReceiver>(mixer), boost::make_shared<SecondReceiver>(mixer, target));
  }
}
