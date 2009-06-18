#ifndef __VORTEX_BASE_H_DEFINED__
#define __VORTEX_BASE_H_DEFINED__

#include "tracking_supp.h"
#include "../devices/aym/aym.h"

namespace ZXTune
{
  namespace Tracking
  {
    struct VortexSample
    {
      VortexSample() : Loop(), Data()
      {
      }

      VortexSample(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
      {
      }

      std::size_t Loop;

      struct Line
      {
        unsigned Level;//0-15
        signed VolSlideAddon;

        bool ToneMask;
        signed ToneOffset;
        bool KeepToneOffset;

        bool NoiseMask;
        bool EnvMask;
        signed NEOffset;
        bool KeepNEOffset;
      };
      std::vector<Line> Data;
    };

    //for unity
    typedef SimpleOrnament VortexOrnament;

    class VortexPlayer : public Tracking::TrackPlayer<3, VortexSample, VortexOrnament>
    {
    private:
      struct Slider
      {
        Slider() : Period(), Value(), Counter(), Delta()
        {
        }
        std::size_t Period;
        signed Value;
        std::size_t Counter;
        signed Delta;

        bool Update()
        {
          if (Counter && !--Counter)
          {
            Value += Delta;
            Counter = Period;
            return true;
          }
          return false;
        }

        void Reset()
        {
          Counter = 0;
          Value = 0;
        }
      };
      struct ChannelState
      {
        ChannelState();
        bool Enabled;
        bool Envelope;
        std::size_t Note;
        std::size_t SampleNum;
        std::size_t PosInSample;
        std::size_t OrnamentNum;
        std::size_t PosInOrnament;
        std::size_t Volume;
        signed VolSlide;
        Slider ToneSlider;
        std::size_t SlidingTargetNote;
        signed ToneAccumulator;
        signed EnvSliding;
        signed NoiseSliding;
        std::size_t VibrateCounter;
        std::size_t VibrateOn;
        std::size_t VibrateOff;
      };
      struct CommonState
      {
        std::size_t EnvBase;
        Slider EnvSlider;
        std::size_t NoiseBase;
        signed NoiseAddon;
      };
    public:
      //commands set
      enum Cmd
      {
        EMPTY,
        GLISS,        //2p: period,delta
        GLISS_NOTE,   //3p: period,delta,note
        SAMPLEOFFSET, //1p: offset
        ORNAMENTOFFSET,//1p:offset
        VIBRATE,      //2p: yestime,notime
        SLIDEENV,     //2p: period,delta
        NOENVELOPE,   //0p
        ENVELOPE,     //2p: type, base
        NOISEBASE,    //1p: base
        TEMPO,        //1p - pseudo-effect
      };

      enum NoteTable
      {
        FREQ_TABLE_PT,
        FREQ_TABLE_ST,
        FREQ_TABLE_ASM,
        FREQ_TABLE_REAL
      };

    protected:
      typedef Tracking::TrackPlayer<3, VortexSample, VortexOrnament> Parent;
      VortexPlayer();
      void Initialize(std::size_t version, NoteTable table);
    public:
      virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const;
      virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver);
      virtual State Reset();
      virtual State SetPosition(const uint32_t& frame);
    private:
      void RenderData(AYM::DataChunk& chunk);
      uint8_t GetVolume(std::size_t volume, std::size_t level);
    protected:
      AYM::Chip::Ptr Device;
      ChannelState Channels[3];
      CommonState Commons;
      std::size_t Version;
      const uint16_t* FreqTable;
      const uint8_t* VolumeTable;
    };
  }
}

#endif //__VORTEX_BASE_H_DEFINED__
