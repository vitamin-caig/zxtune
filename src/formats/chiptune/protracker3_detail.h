/*
Abstract:
  ProTracker v3.xx format detail

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_PROTRACKER3_DETAIL_H_DEFINED
#define FORMATS_CHIPTUNE_PROTRACKER3_DETAIL_H_DEFINED

//local includes
#include "protracker3.h"
//std includes
#include <set>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProTracker3
    {
      typedef std::set<uint_t> Indices;

      class StatisticCollectingBuilder : public Builder
      {
      public:
        explicit StatisticCollectingBuilder(Builder& delegate)
          : Delegate(delegate)
          , Mode(SINGLE_AY_MODE)
        {
        }

        virtual void SetProgram(const String& program)
        {
          return Delegate.SetProgram(program);
        }

        virtual void SetTitle(const String& title)
        {
          return Delegate.SetTitle(title);
        }

        virtual void SetAuthor(const String& author)
        {
          return Delegate.SetAuthor(author);
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
          assert(0 == index || UsedSamples.count(index));
          return Delegate.SetSample(index, sample);
        }

        virtual void SetOrnament(uint_t index, const Ornament& ornament)
        {
          assert(0 == index || UsedOrnaments.count(index));
          return Delegate.SetOrnament(index, ornament);
        }

        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
        {
          UsedPatterns = Indices(positions.begin(), positions.end());
          return Delegate.SetPositions(positions, loop);
        }

        virtual void StartPattern(uint_t index)
        {
          assert(UsedPatterns.count(index) || SINGLE_AY_MODE != Mode);
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

        virtual void SetOrnament(uint_t ornament)
        {
          UsedOrnaments.insert(ornament);
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

#endif //FORMATS_CHIPTUNE_PROTRACKER3_DETAIL_H_DEFINED
