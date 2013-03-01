/*
Abstract:
  DAC-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
#define __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__

//library includes
#include <core/module_types.h>
#include <devices/dac.h>
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
      class TrackParameters
      {
      public:
        typedef boost::shared_ptr<const TrackParameters> Ptr;
        virtual ~TrackParameters() {}

        virtual bool Looped() const = 0;
        virtual Time::Microseconds FrameDuration() const = 0;

        static Ptr Create(Parameters::Accessor::Ptr params);
      };

      class ChannelDataBuilder
      {
      public:
        explicit ChannelDataBuilder(Devices::DAC::DataChunk::ChannelData& data)
          : Data(data)
        {
        }

        void SetEnabled(bool enabled)
        {
          Data.Enabled = enabled;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::ENABLED;
        }

        void SetNote(uint_t note)
        {
          Data.Note = note;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::NOTE;
        }

        void SetNoteSlide(int_t noteSlide)
        {
          Data.NoteSlide = noteSlide;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::NOTESLIDE;
        }

        void SetFreqSlideHz(int_t freqSlideHz)
        {
          Data.FreqSlideHz = freqSlideHz;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::FREQSLIDEHZ;
        }

        void SetSampleNum(uint_t sampleNum)
        {
          Data.SampleNum = sampleNum;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::SAMPLENUM;
        }

        void SetPosInSample(uint_t posInSample)
        {
          Data.PosInSample = posInSample;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::POSINSAMPLE;
        }

        void SetLevelInPercents(uint_t levelInPercents)
        {
          Data.LevelInPercents = levelInPercents;
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::LEVELINPERCENTS;
        }
      private:
        Devices::DAC::DataChunk::ChannelData& Data;
      };

      class TrackBuilder
      {
      public:
        ChannelDataBuilder GetChannel(uint_t chan);

        void GetResult(std::vector<Devices::DAC::DataChunk::ChannelData>& result);
      private:
        std::vector<Devices::DAC::DataChunk::ChannelData> Data;
      };

      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::FixedChannelsReceiver<3>::Ptr target);
      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::FixedChannelsReceiver<4>::Ptr target);

      Analyzer::Ptr CreateAnalyzer(Devices::DAC::Chip::Ptr device);
      Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
