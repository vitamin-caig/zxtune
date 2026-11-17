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

#include "formats/chiptune/digital/digital.h"

#include "tools/indices.h"

namespace Formats::Chiptune::Digital
{
  class StatisticCollectionBuilder
    : public Builder
    , public PatternBuilderProxy<Digital::ChannelBuilder>
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
    Digital::PatternBuilder& StartPattern(uint_t index) override
    {
      BindPattern(Delegate.StartPattern(index));
      return *this;
    }

    void SetRest() override
    {
      return GetChannel().SetRest();
    }

    void SetNote(uint_t note) override
    {
      return GetChannel().SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      UsedSamples.Insert(sample);
      return GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      return GetChannel().SetOrnament(ornament);
    }

    void SetVolume(uint_t volume) override
    {
      return GetChannel().SetVolume(volume);
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
