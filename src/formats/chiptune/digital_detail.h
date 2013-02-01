/*
Abstract:
  Base interfaces for simple digital trackers (only samples, without effects)

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_DIGITAL_DETAIL_H_DEFINED
#define FORMATS_CHIPTUNE_DIGITAL_DETAIL_H_DEFINED

//local includes
#include "digital.h"
//std includes
#include <set>

namespace Formats
{
  namespace Chiptune
  {
    namespace Digital
    {
      typedef std::set<uint_t> Indices;

      class StatisticCollectionBuilder : public Builder
      {
      public:
        explicit StatisticCollectionBuilder(Builder& delegate)
          : Delegate(delegate)
        {
        }

        virtual void SetTitle(const String& title)
        {
          return Delegate.SetTitle(title);
        }

        virtual void SetProgram(const String& program)
        {
          return Delegate.SetProgram(program);
        }

        virtual void SetInitialTempo(uint_t tempo)
        {
          return Delegate.SetInitialTempo(tempo);
        }

        virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr data, bool is4Bit)
        {
          return Delegate.SetSample(index, loop, data, is4Bit);
        }

        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
        {
          UsedPatterns = Indices(positions.begin(), positions.end());
          return Delegate.SetPositions(positions, loop);
        }

        virtual void StartPattern(uint_t index)
        {
          return Delegate.StartPattern(index);
        }

        virtual void StartLine(uint_t index)
        {
          return Delegate.StartLine(index);
        }

        virtual void SetTempo(uint_t tempo)
        {
          return Delegate.SetTempo(tempo);
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
          UsedSamples.insert(sample);
          return Delegate.SetSample(sample);
        }

        const Indices& GetUsedPatterns() const
        {
          return UsedPatterns;
        }

        const Indices& GetUsedSamples() const
        {
          return UsedSamples;
        }
      private:
        Builder& Delegate;
        Indices UsedPatterns;
        Indices UsedSamples;
      };
    }
  }
}

#endif //FORMATS_CHIPTUNE_DIGITAL_DETAIL_H_DEFINED
