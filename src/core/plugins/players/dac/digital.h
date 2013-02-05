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

        typedef TrackingSupport<Channels, uint_t, Devices::DAC::Sample::Ptr, VoidType> Track;

        class Builder : public Formats::Chiptune::Digital::Builder
        {
        public:
          Builder(typename Track::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
            : Data(data)
            , Properties(props)
            , Context(*Data)
          {
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
            Data->Positions.assign(positions.begin(), positions.end());
            Data->LoopPosition = loop;
          }

          virtual void StartPattern(uint_t index)
          {
            Context.SetPattern(index);
          }

          virtual void StartLine(uint_t index)
          {
            Context.SetLine(index);
          }

          virtual void SetTempo(uint_t tempo)
          {
            Context.CurLine->SetTempo(tempo);
          }

          virtual void StartChannel(uint_t index)
          {
            Context.SetChannel(index);
          }

          virtual void SetRest()
          {
            Context.CurChannel->SetEnabled(false);
          }

          virtual void SetNote(uint_t note)
          {
            Context.CurChannel->SetEnabled(true);
            Context.CurChannel->SetNote(note);
          }

          virtual void SetSample(uint_t sample)
          {
            Context.CurChannel->SetSample(sample);
          }
        private:
          const typename Track::ModuleData::RWPtr Data;
          const ModuleProperties::RWPtr Properties;

          typename Track::BuildContext Context;
        };

        class Renderer : public ZXTune::Module::Renderer
        {
        public:
          Renderer(Parameters::Accessor::Ptr params, Information::Ptr info, typename Track::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
            : Data(data)
            , Params(DAC::TrackParameters::Create(params))
            , Device(device)
            , Iterator(CreateTrackStateIterator(info, Data))
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
            std::vector<Devices::DAC::DataChunk::ChannelData> res;
            const TrackState::Ptr state = Iterator->GetStateObserver();
            const typename Track::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
            for (uint_t chan = 0; chan != Channels; ++chan)
            {
              //begin note
              if (line && 0 == state->Quirk())
              {
                const typename Track::Line::Chan& src = line->Channels[chan];
                Devices::DAC::DataChunk::ChannelData dst;
                dst.Channel = chan;
                if (src.Enabled)
                {
                  if (!(dst.Enabled = *src.Enabled))
                  {
                    dst.PosInSample = 0;
                  }
                }
                if (src.Note)
                {
                  dst.Note = *src.Note;
                  dst.PosInSample = 0;
                }
                if (src.SampleNum)
                {
                  dst.SampleNum = *src.SampleNum;
                  dst.PosInSample = 0;
                }
                //store if smth new
                if (dst.Enabled || dst.Note || dst.SampleNum || dst.PosInSample)
                {
                  res.push_back(dst);
                }
              }
            }
            chunk.Channels.swap(res);
          }
        private:
          const typename Track::ModuleData::Ptr Data;
          const DAC::TrackParameters::Ptr Params;
          const Devices::DAC::Chip::Ptr Device;
          const StateIterator::Ptr Iterator;
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
            const Sound::MultichannelMixer::Ptr mixer = Sound::CreateMultichannelMixer(Channels, params);
            mixer->SetTarget(target);
            const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer, Channels);
            const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
            const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(Channels, BaseFreq, chipParams, receiver));
            for (uint_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
            {
              chip->SetSample(idx, Data->Samples[idx]);
            }
            return boost::make_shared<Renderer>(params, Info, Data, chip);
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
