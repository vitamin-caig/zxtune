/*
Abstract:
  Simple digital tracks playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED
#define CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED

//local includes
#include "dac_base.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/tracking.h"
#include "core/plugins/players/module_properties.h"
//library includes
#include <core/module_holder.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/digital.h>
#include <sound/mixer_factory.h>

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
      struct VoidType {};

      template<uint_t Channels>
      struct Digital
      {

        typedef TrackingSupport<Channels, Devices::DAC::Sample::Ptr, VoidType> Track;

        class DataBuilder : public Formats::Chiptune::Digital::Builder
        {
        public:
          DataBuilder(typename Track::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
            : Data(data)
            , Properties(props)
            , Builder(PatternsBuilder::Create<Channels>())
          {
            Data->Patterns = Builder.GetPatterns();
          }

          virtual void SetTitle(const String& title)
          {
            Properties->SetTitle(OptimizeString(title));
          }

          virtual void SetProgram(const String& program)
          {
            Properties->SetProgram(program);
          }

          virtual void SetInitialTempo(uint_t tempo)
          {
            Data->InitialTempo = tempo;
          }

          virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr content, bool is4Bit)
          {
            Data->Samples.resize(index + 1);
            Data->Samples[index] = is4Bit
              ? Devices::DAC::CreateU4Sample(content, loop)
              : Devices::DAC::CreateU8Sample(content, loop);
          }

          virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
          {
            Data->Order = boost::make_shared<SimpleOrderList>(positions.begin(), positions.end(), loop);
          }

          virtual void StartPattern(uint_t index)
          {
            Builder.SetPattern(index);
          }

          virtual void StartLine(uint_t index)
          {
            Builder.SetLine(index);
          }

          virtual void SetTempo(uint_t tempo)
          {
            Builder.GetLine().SetTempo(tempo);
          }

          virtual void StartChannel(uint_t index)
          {
            Builder.SetChannel(index);
          }

          virtual void SetRest()
          {
            Builder.GetChannel().SetEnabled(false);
          }

          virtual void SetNote(uint_t note)
          {
            Builder.GetChannel().SetEnabled(true);
            Builder.GetChannel().SetNote(note);
          }

          virtual void SetSample(uint_t sample)
          {
            Builder.GetChannel().SetSample(sample);
          }
        private:
          const typename Track::ModuleData::RWPtr Data;
          const ModuleProperties::RWPtr Properties;
          PatternsBuilder Builder;
        };

        class Renderer : public ZXTune::Module::Renderer
        {
        public:
          Renderer(Parameters::Accessor::Ptr params, typename Track::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
            : Data(data)
            , Params(DAC::TrackParameters::Create(params))
            , Device(device)
            , Iterator(CreateTrackStateIterator(Data))
            , LastRenderTime(0)
          {
      #ifndef NDEBUG
      //perform self-test
            Devices::DAC::DataChunk chunk;
            while (Iterator->IsValid())
            {
              RenderData(chunk);
              Iterator->NextFrame(false);
            }
            Reset();
      #endif
          }

          virtual TrackState::Ptr GetTrackState() const
          {
            return Iterator->GetStateObserver();
          }

          virtual Analyzer::Ptr GetAnalyzer() const
          {
            return DAC::CreateAnalyzer(Device);
          }

          virtual bool RenderFrame()
          {
            if (Iterator->IsValid())
            {
              LastRenderTime += Params->FrameDuration();
              Devices::DAC::DataChunk chunk;
              RenderData(chunk);
              chunk.TimeStamp = LastRenderTime;
              Device->RenderData(chunk);
              Device->Flush();
              Iterator->NextFrame(Params->Looped());
            }
            return Iterator->IsValid();
          }

          virtual void Reset()
          {
            Device->Reset();
            Iterator->Reset();
            LastRenderTime = Time::Microseconds();
          }

          virtual void SetPosition(uint_t frame)
          {
            const TrackState::Ptr state = Iterator->GetStateObserver();
            if (frame < state->Frame())
            {
              //reset to beginning in case of moving back
              Iterator->Reset();
            }
            //fast forward
            Devices::DAC::DataChunk chunk;
            while (state->Frame() < frame && Iterator->IsValid())
            {
              //do not update tick for proper rendering
              RenderData(chunk);
              Iterator->NextFrame(false);
            }
          }

        private:
          void RenderData(Devices::DAC::DataChunk& chunk)
          {
            const TrackModelState::Ptr state = Iterator->GetStateObserver();
            DAC::TrackBuilder track;
            SynthesizeData(*state, track);
            track.GetResult(chunk.Channels);
          }

          void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
          {
            if (0 == state.Quirk())
            {
              GetNewLineState(state, track);
            }
          }

          void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
          {
            if (const Line::Ptr line = state.LineObject())
            {
              for (uint_t chan = 0; chan != Track::CHANNELS; ++chan)
              {
                if (const Cell::Ptr src = line->GetChannel(chan))
                {
                  ChannelDataBuilder builder = track.GetChannel(chan);
                  GetNewChannelState(*src, builder);
                }
              }
            }
          }

          void GetNewChannelState(const Cell& src, ChannelDataBuilder& builder)
          {
            if (const bool* enabled = src.GetEnabled())
            {
              builder.SetEnabled(*enabled);
              if (!*enabled)
              {
                builder.SetPosInSample(0);
              }
            }
            if (const uint_t* note = src.GetNote())
            {
              builder.SetNote(*note);
              builder.SetPosInSample(0);
            }
            if (const uint_t* sample = src.GetSample())
            {
              builder.SetSampleNum(*sample);
              builder.SetPosInSample(0);
            }
          }
        private:
          const typename Track::ModuleData::Ptr Data;
          const DAC::TrackParameters::Ptr Params;
          const Devices::DAC::Chip::Ptr Device;
          const TrackStateIterator::Ptr Iterator;
          Time::Microseconds LastRenderTime;
        };

        class Holder : public ZXTune::Module::Holder
        {
        public:
          Holder(typename Track::ModuleData::Ptr data, ModuleProperties::Ptr properties, uint_t baseFreq)
            : Data(data)
            , Properties(properties)
            , Info(CreateTrackInfo(Data, Channels))
            , BaseFreq(baseFreq)
          {
          }

          virtual Information::Ptr GetModuleInformation() const
          {
            return Info;
          }

          virtual Parameters::Accessor::Ptr GetModuleProperties() const
          {
            return Properties;
          }

          virtual Module::Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
          {
            const typename Sound::FixedChannelsMixer<Channels>::Ptr mixer = Sound::CreateMixer(params, Sound::FixedChannelsSample<Channels>());
            mixer->SetTarget(target);
            const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer);
            const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
            const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(Channels, BaseFreq, chipParams, receiver));
            for (std::size_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
            {
              chip->SetSample(uint_t(idx), Data->Samples[idx]);
            }
            return boost::make_shared<Renderer>(params, Data, chip);
          }
        private:
          const typename Track::ModuleData::Ptr Data;
          const ModuleProperties::Ptr Properties;
          const Information::Ptr Info;
          const uint_t BaseFreq;
        };
      };
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_DAC_DIGITAL_DEFINED
