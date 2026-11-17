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

#include "formats/chiptune/aym/protracker3.h"

#include "tools/indices.h"

namespace Formats::Chiptune::ProTracker3
{
  const std::size_t MAX_PATTERNS_COUNT = 85;
  const std::size_t MAX_POSITIONS_COUNT = 255;
  const std::size_t MIN_PATTERN_SIZE = 1;
  const std::size_t MAX_PATTERN_SIZE = 256;  // really no limit for PT3.58+
  const std::size_t MAX_SAMPLES_COUNT = 32;
  const std::size_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t SINGLE_AY_MODE = 0x20;

  class StatisticCollectingBuilder
    : public Builder
    , public PatternBuilderProxy<ProTracker3::ChannelBuilder>
  {
  public:
    explicit StatisticCollectingBuilder(Builder& delegate)
      : Delegate(delegate)
      , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
      , AvailablePatterns(0, MAX_PATTERNS_COUNT - 1)
      , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
      , AvailableSamples(0, MAX_SAMPLES_COUNT - 1)
      , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
      , AvailableOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
    {
      UsedSamples.Insert(DEFAULT_SAMPLE);
      UsedOrnaments.Insert(DEFAULT_ORNAMENT);
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return Delegate.GetMetaBuilder();
    }

    void SetVersion(uint_t version) override
    {
      return Delegate.SetVersion(version);
    }

    void SetNoteTable(NoteTable table) override
    {
      return Delegate.SetNoteTable(table);
    }

    void SetMode(uint_t mode) override
    {
      Mode = mode;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      return Delegate.SetInitialTempo(tempo);
    }

    void SetSample(uint_t index, Sample sample) override
    {
      AvailableSamples.Insert(index);
      return Delegate.SetSample(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Ornament ornament) override
    {
      AvailableOrnaments.Insert(index);
      return Delegate.SetOrnament(index, std::move(ornament));
    }

    void SetPositions(Positions positions) override
    {
      UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
      Require(!UsedPatterns.Empty());
      return Delegate.SetPositions(std::move(positions));
    }
    ProTracker3::PatternBuilder& StartPattern(uint_t index) override
    {
      AvailablePatterns.Insert(index);
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

    void SetVolume(uint_t vol) override
    {
      return GetChannel().SetVolume(vol);
    }

    void SetGlissade(uint_t period, int_t val) override
    {
      return GetChannel().SetGlissade(period, val);
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t limit) override
    {
      return GetChannel().SetNoteGliss(period, val, limit);
    }

    void SetSampleOffset(uint_t offset) override
    {
      return GetChannel().SetSampleOffset(offset);
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      return GetChannel().SetOrnamentOffset(offset);
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      return GetChannel().SetVibrate(ontime, offtime);
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      return GetChannel().SetEnvelopeSlide(period, val);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      return GetChannel().SetEnvelope(type, value);
    }

    void SetNoEnvelope() override
    {
      return GetChannel().SetNoEnvelope();
    }

    void SetNoiseBase(uint_t val) override
    {
      return GetChannel().SetNoiseBase(val);
    }

    uint_t GetMode() const
    {
      return Mode;
    }

    const Indices& GetUsedPatterns() const
    {
      return UsedPatterns;
    }

    const Indices& GetAvailablePatterns() const
    {
      return AvailablePatterns;
    }

    const Indices& GetUsedSamples() const
    {
      return UsedSamples;
    }

    const Indices& GetAvailableSamples() const
    {
      return AvailableSamples;
    }

    const Indices& GetUsedOrnaments() const
    {
      return UsedOrnaments;
    }

    const Indices& GetAvailableOrnaments() const
    {
      return AvailableOrnaments;
    }

  private:
    Builder& Delegate;
    uint_t Mode = SINGLE_AY_MODE;
    Indices UsedPatterns;
    Indices AvailablePatterns;
    Indices UsedSamples;
    Indices AvailableSamples;
    Indices UsedOrnaments;
    Indices AvailableOrnaments;
  };

}  // namespace Formats::Chiptune::ProTracker3
