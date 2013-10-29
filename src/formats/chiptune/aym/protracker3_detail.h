/**
* 
* @file
*
* @brief  ProTracker v3.x format details
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "protracker3.h"
//common includes
#include <indices.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProTracker3
    {
      const std::size_t MAX_PATTERNS_COUNT = 85;
      const std::size_t MAX_POSITIONS_COUNT = 255;
      const std::size_t MIN_PATTERN_SIZE = 1;
      const std::size_t MAX_PATTERN_SIZE = 256;//really no limit for PT3.58+
      const std::size_t MAX_SAMPLES_COUNT = 32;
      const std::size_t MAX_ORNAMENTS_COUNT = 16;

      class StatisticCollectingBuilder : public Builder
      {
      public:
        explicit StatisticCollectingBuilder(Builder& delegate)
          : Delegate(delegate)
          , Mode(SINGLE_AY_MODE)
          , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
          , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
          , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
        {
          UsedSamples.Insert(0);
          UsedOrnaments.Insert(0);
        }

        virtual MetaBuilder& GetMetaBuilder()
        {
          return Delegate.GetMetaBuilder();
        }

        virtual void SetVersion(uint_t version)
        {
          return Delegate.SetVersion(version);
        }

        virtual void SetNoteTable(NoteTable table)
        {
          return Delegate.SetNoteTable(table);
        }

        virtual void SetMode(uint_t mode)
        {
          Mode = mode;
          return Delegate.SetMode(mode);
        }

        virtual void SetInitialTempo(uint_t tempo)
        {
          return Delegate.SetInitialTempo(tempo);
        }

        virtual void SetSample(uint_t index, const Sample& sample)
        {
          assert(0 == index || UsedSamples.Contain(index));
          return Delegate.SetSample(index, sample);
        }

        virtual void SetOrnament(uint_t index, const Ornament& ornament)
        {
          assert(0 == index || UsedOrnaments.Contain(index));
          return Delegate.SetOrnament(index, ornament);
        }

        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
        {
          UsedPatterns.Assign(positions.begin(), positions.end());
          Require(!UsedPatterns.Empty());
          return Delegate.SetPositions(positions, loop);
        }

        virtual PatternBuilder& StartPattern(uint_t index)
        {
          assert(UsedPatterns.Contain(index) || SINGLE_AY_MODE != Mode);
          return Delegate.StartPattern(index);
        }

        virtual void StartChannel(uint_t index)
        {
          return Delegate.StartChannel(index);
        }

        virtual void SetRest()
        {
          return Delegate.SetRest();
        }

        virtual void SetNote(uint_t note)
        {
          return Delegate.SetNote(note);
        }

        virtual void SetSample(uint_t sample)
        {
          UsedSamples.Insert(sample);
          return Delegate.SetSample(sample);
        }

        virtual void SetOrnament(uint_t ornament)
        {
          UsedOrnaments.Insert(ornament);
          return Delegate.SetOrnament(ornament);
        }

        virtual void SetVolume(uint_t vol)
        {
          return Delegate.SetVolume(vol);
        }

        virtual void SetGlissade(uint_t period, int_t val)
        {
          return Delegate.SetGlissade(period, val);
        }

        virtual void SetNoteGliss(uint_t period, int_t val, uint_t limit)
        {
          return Delegate.SetNoteGliss(period, val, limit);
        }

        virtual void SetSampleOffset(uint_t offset)
        {
          return Delegate.SetSampleOffset(offset);
        }

        virtual void SetOrnamentOffset(uint_t offset)
        {
          return Delegate.SetOrnamentOffset(offset);
        }

        virtual void SetVibrate(uint_t ontime, uint_t offtime)
        {
          return Delegate.SetVibrate(ontime, offtime);
        }

        virtual void SetEnvelopeSlide(uint_t period, int_t val)
        {
          return Delegate.SetEnvelopeSlide(period, val);
        }

        virtual void SetEnvelope(uint_t type, uint_t value)
        {
          return Delegate.SetEnvelope(type, value);
        }

        virtual void SetNoEnvelope()
        {
          return Delegate.SetNoEnvelope();
        }

        virtual void SetNoiseBase(uint_t val)
        {
          return Delegate.SetNoiseBase(val);
        }

        uint_t GetMode() const
        {
          return Mode;
        }

        const Indices& GetUsedPatterns() const
        {
          return UsedPatterns;
        }

        const Indices& GetUsedSamples() const
        {
          return UsedSamples;
        }

        const Indices& GetUsedOrnaments() const
        {
          return UsedOrnaments;
        }
      private:
        Builder& Delegate;
        uint_t Mode;
        Indices UsedPatterns;
        Indices UsedSamples;
        Indices UsedOrnaments;
      };
    }
  }
}
