/*
Abstract:
  SoundTrackerPro format details

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_SOUNDTRACKERPRO_DETAIL_H_DEFINED
#define FORMATS_CHIPTUNE_SOUNDTRACKERPRO_DETAIL_H_DEFINED

//local includes
#include "soundtracker.h"
//common includes
#include <indices.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SoundTrackerPro
    {
      const uint_t MAX_SAMPLE_SIZE = 32;
      const uint_t MAX_ORNAMENT_SIZE = 32;
      const uint_t MIN_PATTERN_SIZE = 5;
      const uint_t MAX_PATTERN_SIZE = 64;
      const uint_t MAX_POSITIONS_COUNT = 256;
      const uint_t MAX_PATTERNS_COUNT = 32;
      const uint_t MAX_SAMPLES_COUNT = 15;
      const uint_t MAX_ORNAMENTS_COUNT = 16;

      class StatisticCollectingBuilder : public Builder
      {
      public:
        explicit StatisticCollectingBuilder(Builder& delegate)
          : Delegate(delegate)
          , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
          , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
          , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
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
          return Delegate.SetSample(index, sample);
        }

        virtual void SetOrnament(uint_t index, const Ornament& ornament)
        {
          assert(0 == index || UsedOrnaments.Contain(index));
          return Delegate.SetOrnament(index, ornament);
        }

        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop)
        {
          Require(!positions.empty());
          UsedPatterns.Clear();
          for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
          {
            UsedPatterns.Insert(it->PatternIndex);
          }
          return Delegate.SetPositions(positions, loop);
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

        virtual void SetEnvelope(uint_t type, uint_t value)
        {
          return Delegate.SetEnvelope(type, value);
        }

        virtual void SetNoEnvelope()
        {
          return Delegate.SetNoEnvelope();
        }

        virtual void SetGliss(uint_t target)
        {
          return Delegate.SetGliss(target);
        }

        virtual void SetVolume(uint_t vol)
        {
          return Delegate.SetVolume(vol);
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
      private:
        Builder& Delegate;
        Indices UsedPatterns;
        Indices UsedSamples;
        Indices UsedOrnaments;
      };
    }
  }
}

#endif //FORMATS_CHIPTUNE_SOUNDTRACKERPRO_DETAIL_H_DEFINED
