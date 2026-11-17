/**
 *
 * @file
 *
 * @brief  FastTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/fasttracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::FastTracker
{
  using namespace Formats::Chiptune::FastTracker;

  StringView NoteTableName(NoteTable table)
  {
    switch (table)
    {
    case NoteTable::PROTRACKER2:
      return "PROTRACKER2"sv;
    case NoteTable::SOUNDTRACKER:
      return "SOUNDTRACKER"sv;
    case NoteTable::FASTTRACKER:
      return "FASTTRACKER"sv;
    default:
      return "UNKNOWN"sv;
    }
  }

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetEnvelope(uint_t type, uint_t tone) override
    {
      Command("Envelope"sv, "type"sv, type, "tone"sv, tone);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
    }

    void SetNoise(uint_t val) override
    {
      Command("Noise"sv, val);
    }

    void SetRetrig() override
    {
      Command("Retrig"sv);
    }

    void SetSlide(int_t step) override
    {
      Command("Slide"sv, step);
    }

    void SetNoteSlide(uint_t step) override
    {
      Command("NoteSlide"sv, step);
    }
  };

  class Dumper : public Builder
  {
  public:
    explicit Dumper(StringView decoder)
    {
      Dump::Header(decoder, 3);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetNoteTable(NoteTable table) override
    {
      Dump::Scalar(0, "note_table"sv, NoteTableName(table));
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Dump::Number(0, "initial_tempo"sv, tempo);
    }

    void SetSample(uint_t /*index*/, Sample /*sample*/) override {}

    void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}

    void SetPositions(Positions positions) override
    {
      Dump::DumpPositions(positions, [](const PositionEntry& entry) { Dump::DumpPositionEntry(entry); });
    }

    PatternBuilder& StartPattern(uint_t index) override
    {
      return Patterns.Start(index);
    }

  private:
    Dump::MetaDumper Meta;
    Dump::PatternDumper<ChannelBuilder, ChannelDumper> Patterns;
  };
}  // namespace Dump::FastTracker

bool DumpFastTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::FastTracker::Dumper>(data, &Formats::Chiptune::CreateFastTrackerDecoder,
                                                       &Formats::Chiptune::FastTracker::Parse);
}
