/*
Abstract:
  SoundTracker format details

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_SOUNDTRACKER_DETAIL_H_DEFINED
#define FORMATS_CHIPTUNE_SOUNDTRACKER_DETAIL_H_DEFINED

//local includes
#include "soundtracker.h"
//common includes
#include <indices.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SoundTracker
    {
      const uint_t SAMPLE_SIZE = 32;
      const uint_t ORNAMENT_SIZE = 32;
      const uint_t MIN_PATTERN_SIZE = 5;
      const uint_t MAX_PATTERN_SIZE = 64;
      const uint_t MAX_POSITIONS_COUNT = 256;
      const uint_t MAX_PATTERNS_COUNT = 32;
      //0 usually is empty
      const uint_t MAX_SAMPLES_COUNT = 16;
      const uint_t MAX_ORNAMENTS_COUNT = 16;

      class StatisticCollectingBuilder : public Builder
      {
      public:
        explicit StatisticCollectingBuilder(Builder& delegate)
          : Delegate(delegate)
          , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
          , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
          , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
          , NonEmptyPatterns(false)
          , NonEmptySamples(false)
        {
          UsedOrnaments.Insert(0);
        }

        virtual void SetProgram(const String& program)
        {
          return Delegate.SetProgram(program);
        }

        virtual void SetTitle(const String& title)
        {
          return Delegate.SetTitle(title);
        }

        virtual void SetInitialTempo(uint_t tempo)
        {
          return Delegate.SetInitialTempo(tempo);
        }

        virtual void SetSample(uint_t index, const Sample& sample)
        {
          assert(UsedSamples.Contain(index));
          if (IsSampleSounds(sample))
          {
            NonEmptySamples = true;
          }
          return Delegate.SetSample(index, sample);
        }

        virtual void SetOrnament(uint_t index, const Ornament& ornament)
        {
          assert(UsedOrnaments.Contain(index));
          return Delegate.SetOrnament(index, ornament);
        }

        virtual void SetPositions(const std::vector<PositionEntry>& positions)
        {
          Require(!positions.empty());
          UsedPatterns.Clear();
          for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
          {
            UsedPatterns.Insert(it->PatternIndex);
          }
          return Delegate.SetPositions(positions);
        }

        virtual void StartPattern(uint_t index)
        {
          assert(UsedPatterns.Contain(index));
          return Delegate.StartPattern(index);
        }

        virtual void FinishPattern(uint_t size)
        {
          return Delegate.FinishPattern(size);
        }

        virtual void StartLine(uint_t index)
        {
          return Delegate.StartLine(index);
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
          NonEmptyPatterns = true;
          return Delegate.SetNote(note);
        }

        virtual void SetSample(uint_t sample)
        {
          if (0 != sample)
          {
            NonEmptyPatterns = true;
          }
          UsedSamples.Insert(sample);
          return Delegate.SetSample(sample);
        }

        virtual void SetOrnament(uint_t ornament)
        {
          UsedOrnaments.Insert(ornament);
          return Delegate.SetOrnament(ornament);
        }

        virtual void SetEnvelope(uint_t type, uint_t value)
        {
          NonEmptyPatterns = true;
          return Delegate.SetEnvelope(type, value);
        }

        virtual void SetNoEnvelope()
        {
          NonEmptyPatterns = true;
          return Delegate.SetNoEnvelope();
        }

        const Indices& GetUsedPatterns() const
        {
          return UsedPatterns;
        }

        const Indices& GetUsedSamples() const
        {
          Require(!UsedSamples.Empty());
          return UsedSamples;
        }

        const Indices& GetUsedOrnaments() const
        {
          return UsedOrnaments;
        }

        bool HasNonEmptyPatterns() const
        {
          return NonEmptyPatterns;
        }

        bool HasNonEmptySamples() const
        {
          return NonEmptySamples;
        }
      private:
        static bool IsSampleSounds(const Sample& smp)
        {
          if (smp.Lines.empty())
          {
            return false;
          }
          for (uint_t idx = 0; idx != smp.Lines.size(); ++idx)
          {
            const Sample::Line& line = smp.Lines[idx];
            if (line.EnvelopeMask || line.Level)
            {
              return true;//has envelope or tone with volume
            }
          }
          return false;
        }
      private:
        Builder& Delegate;
        Indices UsedPatterns;
        Indices UsedSamples;
        Indices UsedOrnaments;
        bool NonEmptyPatterns;
        bool NonEmptySamples;
      };
    }
  }
}

#endif //FORMATS_CHIPTUNE_SOUNDTRACKER_DETAIL_H_DEFINED
