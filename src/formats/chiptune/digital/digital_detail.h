/**
 *
 * @file
 *
 * @brief  Simple digital trackers format details
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "formats/chiptune/digital/digital.h"
// common includes
#include <tools/indices.h>

namespace Formats::Chiptune::Digital
{
  class StatisticCollectionBuilder : public Builder
  {
  public:
    StatisticCollectionBuilder(Builder& delegate, uint_t maxPatternsCount, uint_t maxSamplesCount)
      : Delegate(delegate)
      , UsedPatterns(0, maxPatternsCount - 1)
      , UsedSamples(0, maxSamplesCount - 1)
    {}

    MetaBuilder& GetMetaBuilder() override
    {
      return Delegate.GetMetaBuilder();
    }

    void SetInitialTempo(uint_t tempo) override
    {
      return Delegate.SetInitialTempo(tempo);
    }

    void SetSamplesFrequency(uint_t freq) override
    {
      return Delegate.SetSamplesFrequency(freq);
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View data, bool is4Bit) override
    {
      return Delegate.SetSample(index, loop, data, is4Bit);
    }

    void SetPositions(Positions positions) override
    {
      UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
      Require(!UsedPatterns.Empty());
      return Delegate.SetPositions(std::move(positions));
    }

    PatternBuilder& StartPattern(uint_t index) override
    {
      return Delegate.StartPattern(index);
    }

    void StartChannel(uint_t index) override
    {
      return Delegate.StartChannel(index);
    }

    void SetRest() override
    {
      return Delegate.SetRest();
    }

    void SetNote(uint_t note) override
    {
      return Delegate.SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      UsedSamples.Insert(sample);
      return Delegate.SetSample(sample);
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

  private:
    Builder& Delegate;
    Indices UsedPatterns;
    Indices UsedSamples;
  };
}  // namespace Formats::Chiptune::Digital
