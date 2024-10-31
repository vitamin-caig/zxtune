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

#include "module/players/iterator.h"
#include "module/players/track_model.h"
#include <formats/chiptune/builder_pattern.h>

#include <module/track_information.h>
#include <module/track_state.h>

#include <make_ptr.h>

#include <algorithm>
#include <array>

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
    using Ptr = std::unique_ptr<MutableLine>;

    virtual void SetTempo(uint_t val) = 0;
    virtual MutableCell& AddChannel(uint_t idx) = 0;
  };

  class MutablePattern : public Pattern
  {
  public:
    using Ptr = std::unique_ptr<MutablePattern>;

    virtual MutableLine& AddLine(uint_t row) = 0;
    virtual void SetSize(uint_t size) = 0;
  };

  class MutablePatternsSet : public PatternsSet
  {
  public:
    using Ptr = std::unique_ptr<MutablePatternsSet>;

    virtual MutablePattern& AddPattern(uint_t idx) = 0;
  };

  template<uint_t ChannelsCount>
  class MultichannelMutableLine : public MutableLine
  {
  public:
    MultichannelMutableLine() = default;

    const Cell* GetChannel(uint_t idx) const override
    {
      return Channels[idx].HasData() ? &Channels[idx] : nullptr;
    }

    uint_t CountActiveChannels() const override
    {
      return static_cast<uint_t>(
          std::count_if(Channels.begin(), Channels.end(), [](const MutableCell& cell) { return cell.HasData(); }));
    }

    uint_t GetTempo() const override
    {
      return Tempo;
    }

    void SetTempo(uint_t val) override
    {
      Tempo = val;
    }

    MutableCell& AddChannel(uint_t idx) override
    {
      return Channels[idx];
    }

  private:
    uint_t Tempo = 0;
    using ChannelsArray = std::array<MutableCell, ChannelsCount>;
    ChannelsArray Channels;
  };

  template<class T>
  class SparsedObjectsStorage
  {
  public:
    const T& Get(uint_t idx) const
    {
      if (idx < Objects.size())
      {
        return Objects[idx];
      }
      else
      {
        static const T STUB;
        return STUB;
      }
    }

    uint_t Size() const
    {
      return Objects.size();
    }

    void Resize(uint_t newSize)
    {
      assert(newSize >= Objects.size());
      Objects.resize(newSize);
    }

    const T& Add(uint_t idx, T obj)
    {
      if (idx >= Objects.size())
      {
        Objects.resize(idx + 1);
      }
      return Objects[idx] = std::move(obj);
    }

  private:
    std::vector<T> Objects;
  };

  template<class MutableLineType>
  class SparsedMutablePattern : public MutablePattern
  {
  public:
    const Line* GetLine(uint_t row) const override
    {
      return Storage.Get(row).get();
    }

    uint_t GetSize() const override
    {
      return Storage.Size();
    }

    MutableLine& AddLine(uint_t row) override
    {
      return *Storage.Add(row, MakePtr<MutableLineType>());
    }

    void SetSize(uint_t newSize) override
    {
      Storage.Resize(newSize);
    }

  private:
    SparsedObjectsStorage<MutableLine::Ptr> Storage;
  };

  template<class MutablePatternType>
  class SparsedMutablePatternsSet : public MutablePatternsSet
  {
  public:
    const class Pattern* Get(uint_t idx) const override
    {
      return Storage.Get(idx).get();
    }

    uint_t GetSize() const override
    {
      uint_t res = 0;
      for (uint_t idx = 0; idx != Storage.Size(); ++idx)
      {
        if (auto* const pat = Storage.Get(idx).get())
        {
          res += pat->GetSize() != 0;
        }
      }
      return res;
    }

    MutablePattern& AddPattern(uint_t idx) override
    {
      return *Storage.Add(idx, MakePtr<MutablePatternType>());
    }

  private:
    SparsedObjectsStorage<MutablePattern::Ptr> Storage;
  };

  TrackInformation::Ptr CreateTrackInfoFixedChannels(Time::Microseconds frameDuration, TrackModel::Ptr model,
                                                     uint_t channels);

  inline TrackInformation::Ptr CreateTrackInfo(Time::Microseconds frameDuration, TrackModel::Ptr model)
  {
    const auto channels = model->GetChannelsCount();
    return CreateTrackInfoFixedChannels(frameDuration, std::move(model), channels);
  }

  class TrackStateIterator : public Iterator
  {
  public:
    using Ptr = std::shared_ptr<TrackStateIterator>;

    virtual TrackModelState::Ptr GetStateObserver() const = 0;
  };

  TrackStateIterator::Ptr CreateTrackStateIterator(Time::Microseconds frameDuration, TrackModel::Ptr model);

  class PatternsBuilder : public Formats::Chiptune::PatternBuilder
  {
  public:
    explicit PatternsBuilder(MutablePatternsSet::Ptr patterns)
      : Patterns(std::move(patterns))
    {}

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
      CurPattern = &Patterns->AddPattern(idx);
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

    PatternsSet::Ptr CaptureResult()
    {
      return std::move(Patterns);
    }

    template<uint_t ChannelsCount>
    static PatternsBuilder Create()
    {
      using LineType = MultichannelMutableLine<ChannelsCount>;
      using PatternType = SparsedMutablePattern<LineType>;
      using PatternsSetType = SparsedMutablePatternsSet<PatternType>;
      return PatternsBuilder(MakePtr<PatternsSetType>());
    }

  private:
    MutablePatternsSet::Ptr Patterns;
    MutablePattern* CurPattern;
    MutableLine* CurLine = nullptr;
    MutableCell* CurChannel = nullptr;
  };
}  // namespace Module
