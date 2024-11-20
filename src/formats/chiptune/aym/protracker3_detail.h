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

  class StatisticCollectingBuilder : public Builder
  {
  public:
    explicit StatisticCollectingBuilder(Builder& delegate)
      : Delegate(delegate)
      , Mode(SINGLE_AY_MODE)
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
      return Delegate.SetMode(mode);
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

    PatternBuilder& StartPattern(uint_t index) override
    {
      AvailablePatterns.Insert(index);
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

    void SetOrnament(uint_t ornament) override
    {
      UsedOrnaments.Insert(ornament);
      return Delegate.SetOrnament(ornament);
    }

    void SetVolume(uint_t vol) override
    {
      return Delegate.SetVolume(vol);
    }

    void SetGlissade(uint_t period, int_t val) override
    {
      return Delegate.SetGlissade(period, val);
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t limit) override
    {
      return Delegate.SetNoteGliss(period, val, limit);
    }

    void SetSampleOffset(uint_t offset) override
    {
      return Delegate.SetSampleOffset(offset);
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      return Delegate.SetOrnamentOffset(offset);
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      return Delegate.SetVibrate(ontime, offtime);
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      return Delegate.SetEnvelopeSlide(period, val);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      return Delegate.SetEnvelope(type, value);
    }

    void SetNoEnvelope() override
    {
      return Delegate.SetNoEnvelope();
    }

    void SetNoiseBase(uint_t val) override
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
    uint_t Mode;
    Indices UsedPatterns;
    Indices AvailablePatterns;
    Indices UsedSamples;
    Indices AvailableSamples;
    Indices UsedOrnaments;
    Indices AvailableOrnaments;
  };
}  // namespace Formats::Chiptune::ProTracker3
