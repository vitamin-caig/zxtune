/**
* 
* @file
*
* @brief  MIDI device support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "roms.h"
//common includes
#include <contract.h>
//library includes
#include <devices/midi.h>
#include <sound/chunk_builder.h>
#include <sound/resampler.h>
#include <time/oscillator.h>
//3rdparty includes
#include <3rdparty/mt32emu/src/mt32emu.h>

namespace Devices
{
  namespace MIDI
  {
    class StubReportHandler : public MT32Emu::ReportHandler
    {
    public:
      virtual void printDebug(const char* /*fmt*/, va_list /*list*/) {}
      virtual void showLCDMessage(const char */*message*/) {}
      
      static MT32Emu::ReportHandler* Instance()
      {
        static StubReportHandler INSTANCE;
        return &INSTANCE;
      }
    };
    
    class MT32Chip : public Chip
    {
    public:
      MT32Chip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
        : Params(params)
        , Synth(StubReportHandler::Instance())
      {
        Open();
        Target = Sound::CreateResampler(Synth.getStereoOutputSampleRate(), Params->SoundFreq(), target);
      }
      
      virtual void RenderData(const DataChunk& src)
      {
        FlushFrame(src.TimeStamp);
        Require(src.Sizes.empty() == src.Data.empty());
        std::size_t offset = 0;
        for (std::size_t idx = 0, lim = src.Sizes.size(); idx != lim; ++idx)
        {
          const std::size_t size = src.Sizes[idx];
          //at least one sample should be between messages
          AddFrameData(&src.Data.front() + offset, size, Snd.GetCurrentTick() + idx);
          offset += size;
        }
      }

      virtual void Reset()
      {
        Synth.close();
        Snd.Reset();
        Open();
      }

      virtual void GetState(MultiChannelState& result) const
      {
        MultiChannelState tmp;
        const uint_t partials = Synth.getPartialCount();
        std::vector<uint8_t> keys(partials);
        std::vector<uint8_t> velocities(partials);
        //ignore rhythm part
        for (unsigned partNumber = 0; partNumber < 8; ++partNumber)
        {
          for (unsigned note = 0, notes = Synth.getPlayingNotes(partNumber, &keys.front(), &velocities.front()); note != notes; ++note)
          {
            //internal key value is in range 12-108
            tmp.push_back(ChannelState(keys[note] - 12, LevelType(velocities[note], 127)));
          }
        }
        result.swap(tmp);
      }
    private:
      void Open()
      {
        Require(Synth.open(*Detail::GetControlROM(), *Detail::GetPCMROM(), MT32Emu::AnalogOutputMode_DIGITAL_ONLY));
        Synth.setMIDIDelayMode(MT32Emu::MIDIDelayMode_IMMEDIATE);
        Synth.setReverbEnabled(false);
        Synth.setReverbOverridden(true);
        //Synth is clocked by native rate
        Snd.SetFrequency(Synth.getStereoOutputSampleRate());
      }
      
      void AddFrameData(const uint8_t* data, std::size_t size, uint_t stamp)
      {
        Require(size != 0);
        if (IsSysex(data[0]))
        {
          Synth.playSysex(data, size, stamp);
        }
        else
        {
          Synth.playMsg(MakeMsg(data, size), stamp);
        }
      }
      
      void FlushFrame(Stamp time)
      {
        if (const uint_t samplesCount = Snd.GetTicksToTime(time))
        {
          Sound::ChunkBuilder builder;
          builder.Reserve(samplesCount);
          Synth.render(safe_ptr_cast<MT32Emu::Sample*>(builder.Allocate(samplesCount)), samplesCount);
          Target->ApplyData(builder.GetResult());
          Target->Flush();
          Snd.AdvanceTick(samplesCount);
        }
      }
      
      static bool IsSysex(uint8_t cmd)
      {
        return cmd == 0xf0;
      }
      
      static MT32Emu::Bit32u MakeMsg(const uint8_t* data, std::size_t size)
      {
        Require(size == 3);
        return uint_t(data[0]) | (uint_t(data[1]) << 8) | (uint_t(data[2]) << 16);
      }
    private:
      const ChipParameters::Ptr Params;
      MT32Emu::Synth Synth;
      Sound::Receiver::Ptr Target;
      Time::Oscillator<Stamp> Snd;
    };
    
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<MT32Chip>(params, target);
    }
  }
}
