/**
 *
 * @file
 *
 * @brief  SoundTrackerPro format details
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/aym/soundtrackerpro.h"

#include "tools/indices.h"

#include <cassert>

namespace Formats::Chiptune::SoundTrackerPro
{
  const uint_t MAX_SAMPLE_SIZE = 32;
  const uint_t MAX_ORNAMENT_SIZE = 32;
  const uint_t MIN_PATTERN_SIZE = 5;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_POSITIONS_COUNT = 256;
  const uint_t MAX_PATTERNS_COUNT = 32;
  const uint_t MAX_SAMPLES_COUNT = 15;
  const uint_t MAX_ORNAMENTS_COUNT = 16;

  class StatisticCollectingBuilder
    : public Builder
    , public PatternBuilderProxy<SoundTrackerPro::ChannelBuilder>
  {
  public:
    explicit StatisticCollectingBuilder(Builder& delegate)
      : Delegate(delegate)
      , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
      , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
      , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
    {
      UsedSamples.Insert(DEFAULT_SAMPLE);
      UsedOrnaments.Insert(DEFAULT_ORNAMENT);
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return Delegate.GetMetaBuilder();
    }

    void SetInitialTempo(uint_t tempo) override
    {
      return Delegate.SetInitialTempo(tempo);
    }

    void SetSample(uint_t index, Sample sample) override
    {
      assert(UsedSamples.Contain(index));
      return Delegate.SetSample(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Ornament ornament) override
    {
      assert(UsedOrnaments.Contain(index));
      return Delegate.SetOrnament(index, std::move(ornament));
    }

    void SetPositions(Positions positions) override
    {
      UsedPatterns.Clear();
      for (const auto& pos : positions.Lines)
      {
        UsedPatterns.Insert(pos.PatternIndex);
      }
      Require(!UsedPatterns.Empty());
      return Delegate.SetPositions(std::move(positions));
    }
    SoundTrackerPro::PatternBuilder& StartPattern(uint_t index) override
    {
      assert(UsedPatterns.Contain(index));
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
      UsedOrnaments.Insert(ornament);
      return GetChannel().SetOrnament(ornament);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      return GetChannel().SetEnvelope(type, value);
    }

    void SetGliss(int_t step) override
    {
      return GetChannel().SetGliss(step);
    }

    void SetVolume(uint_t vol) override
    {
      return GetChannel().SetVolume(vol);
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

}  // namespace Formats::Chiptune::SoundTrackerPro
