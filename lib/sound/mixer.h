#ifndef __MIXER_H_DEFINED__
#define __MIXER_H_DEFINED__

#include <sound.h>

#include <cassert>

namespace ZXTune
{
  namespace Sound
  {
    class MixerManager
    {
    public:
      enum Type
      {
        //3 channels
        MIXER_AYM,
        //1 channel
        MIXER_BEEPER,
        //4 channels
        MIXER_SOUNDRIVE,
      };

      typedef std::auto_ptr<MixerManager> Ptr;

      virtual ~MixerManager()
      {
      }

      virtual void GetMatrix(Type type, Sample* output) const = 0;
      virtual void SetMatrix(Type type, const Sample* input) = 0;

      virtual Receiver::Ptr CreateMixer(Type type, Receiver* receiver);

      static Ptr CreateMixerManager();
    };
  }
}

#endif //__MIXER_H_DEFINED__
