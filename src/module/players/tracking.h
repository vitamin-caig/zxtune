/**
 *
 * @file
 *
 * @brief  Track modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_pattern.h"
#include "module/players/iterator.h"
#include "module/players/track_model.h"

#include "module/information.h"

#include "make_ptr.h"

#include <algorithm>

namespace Module
{
  class MutableCell : public Cell
  {
  public:
    void SetEnabled(bool val)
    {
      Mask |= ENABLED;
      Enabled = val;
    }

    void SetNote(uint_t val)
    {
      Mask |= NOTE;
      Note = val;
    }

    void SetSample(uint_t val)
    {
      Mask |= SAMPLENUM;
      SampleNum = val;
    }

    void SetOrnament(uint_t val)
    {
      Mask |= ORNAMENTNUM;
      OrnamentNum = val;
    }

    void SetVolume(uint_t val)
    {
      Mask |= VOLUME;
      Volume = val;
    }

    void AddCommand(uint_t type, int_t p1 = 0, int_t p2 = 0, int_t p3 = 0)
    {
      Commands.emplace_back(type, p1, p2, p3);
    }

    Command* FindCommand(uint_t type)
    {
      const auto it = std::find(Commands.begin(), Commands.end(), type);
      return it != Commands.end() ? &*it : nullptr;
    }
  };

  class MutableLine : public Line
  {
  public:
    MutableLine() = default;

    void SetTempo(uint_t val)
    {
      Tempo = val;
    }

    auto& AddChannel(uint_t idx)
    {
      return Channels.Add<MutableCell>(idx);
    }
  };

  class MutablePattern : public Pattern
  {
  public:
    auto& AddLine(uint_t row)
    {
      return Lines.Add<MutableLine>(row);
    }

    void SetSize(uint_t newSize)
    {
      Lines.Resize(newSize);
    }
  };

  class MutablePatternsSet : public PatternsSet
  {
  public:
    auto& AddPattern(uint_t idx)
    {
      return Patterns.Add<MutablePattern>(idx);
    }
  };

  Information CreateTrackInfoFixedChannels(Time::Microseconds frameDuration, const TrackModel& model, uint_t channels);

  inline Information CreateTrackInfo(Time::Microseconds frameDuration, const TrackModel& model)
  {
    const auto channels = model.GetChannelsCount();
    return CreateTrackInfoFixedChannels(frameDuration, model, channels);
  }

  Iterator::Ptr CreateTrackStateIterator(Time::Microseconds frameDuration, TrackModel::Ptr model);

  class PatternsBuilder : public Formats::Chiptune::PatternBuilder
  {
  public:
    PatternsBuilder() = default;
    PatternsBuilder(const PatternsBuilder&) = delete;
    PatternsBuilder& operator=(const PatternsBuilder&) = delete;

    PatternsBuilder(PatternsBuilder&& rh) noexcept = default;

    void Finish(uint_t size) override
    {
      FinishPattern(size);
    }

    void StartLine(uint_t index) override
    {
      SetLine(index);
    }

    void SetTempo(uint_t tempo) override
    {
      GetLine().SetTempo(tempo);
    }

    void SetPattern(uint_t idx)
    {
      CurPattern = &Patterns.AddPattern(idx);
      CurLine = nullptr;
      CurChannel = nullptr;
    }

    void SetLine(uint_t idx)
    {
      CurLine = &CurPattern->AddLine(idx);
      CurChannel = nullptr;
    }

    void SetChannel(uint_t idx)
    {
      CurChannel = &CurLine->AddChannel(idx);
    }

    void FinishPattern(uint_t size)
    {
      CurPattern->SetSize(size);
      CurLine = nullptr;
      CurPattern = nullptr;
    }

    MutableLine& GetLine() const
    {
      return *CurLine;
    }

    MutableCell& GetChannel() const
    {
      return *CurChannel;
    }

    PatternsSet CaptureResult()
    {
      return std::move(Patterns);
    }

  private:
    MutablePatternsSet Patterns;
    MutablePattern* CurPattern;
    MutableLine* CurLine = nullptr;
    MutableCell* CurChannel = nullptr;
  };
}  // namespace Module
