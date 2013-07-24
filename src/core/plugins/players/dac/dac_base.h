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

//local includes
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>
#include <devices/dac.h>
#include <sound/receiver.h>
#include <sound/render_params.h>

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
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

        void DropPosInSample()
        {
          Data.Mask &= ~Devices::DAC::DataChunk::ChannelData::POSINSAMPLE;
        }

        void SetLevelInPercents(uint_t levelInPercents)
        {
          Data.Level = Devices::LevelType(levelInPercents, Devices::LevelType::PRECISION);
          Data.Mask |= Devices::DAC::DataChunk::ChannelData::LEVEL;
        }

        Devices::DAC::DataChunk::ChannelData& GetState() const
        {
          return Data;
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

      class DataRenderer
      {
      public:
        typedef boost::shared_ptr<DataRenderer> Ptr;

        virtual ~DataRenderer() {}

        virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
        virtual void Reset() = 0;
      };

      class DataIterator : public StateIterator
      {
      public:
        typedef boost::shared_ptr<DataIterator> Ptr;

        virtual void GetData(Devices::DAC::DataChunk& data) const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual Parameters::Accessor::Ptr GetProperties() const = 0;
        virtual DataIterator::Ptr CreateDataIterator() const = 0;
        virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const = 0;
      };

      DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

      Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr chip);

      Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
