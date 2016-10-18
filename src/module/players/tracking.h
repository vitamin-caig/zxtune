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

//local includes
#include "iterator.h"
#include "track_model.h"
//common includes
#include <make_ptr.h>
//library includes
#include <formats/chiptune/builder_pattern.h>
#include <module/information.h>
#include <module/track_state.h>
//std includes
#include <algorithm>
#include <array>
#include <functional>

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
      Commands.push_back(Command(type, p1, p2, p3));
    }

    Command* FindCommand(uint_t type)
    {
      const CommandsArray::iterator it = std::find(Commands.begin(), Commands.end(), type);
      return it != Commands.end()
        ? &*it
        : 0;
    }
  };

  class MutableLine : public Line
  {
  public:
    typedef std::shared_ptr<MutableLine> Ptr;
    
    virtual void SetTempo(uint_t val) = 0;
    virtual MutableCell& AddChannel(uint_t idx) = 0;
  };

  class MutablePattern : public Pattern
  {
  public:
    typedef std::shared_ptr<MutablePattern> Ptr;
    
    virtual MutableLine& AddLine(uint_t row) = 0;
    virtual void SetSize(uint_t size) = 0;
  };

  class MutablePatternsSet : public PatternsSet
  {
  public:
    typedef std::shared_ptr<MutablePatternsSet> Ptr;
    
    virtual MutablePattern& AddPattern(uint_t idx) = 0;
  };

  template<uint_t ChannelsCount>
  class MultichannelMutableLine : public MutableLine
  {
  public:
    MultichannelMutableLine()
      : Tempo()
    {
    }

    Cell::Ptr GetChannel(uint_t idx) const override
    {
      return Channels[idx].HasData() ? &Channels[idx] : 0;
    }

    uint_t CountActiveChannels() const override
    {
      return static_cast<uint_t>(std::count_if(Channels.begin(), Channels.end(), std::mem_fn(&Cell::HasData)));
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
    uint_t Tempo;
    typedef std::array<MutableCell, ChannelsCount> ChannelsArray;
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

    void Add(uint_t idx, const T& obj)
    {
      if (idx >= Objects.size())
      {
        Objects.resize(idx + 1);
      }
      Objects[idx] = obj;
    }
  private:
    std::vector<T> Objects;
  };

  template<class MutableLineType>
  class SparsedMutablePattern : public MutablePattern
  {
  public:
    Line::Ptr GetLine(uint_t row) const override
    {
      return Storage.Get(row);
    }

    uint_t GetSize() const override
    {
      return Storage.Size();
    }

    MutableLine& AddLine(uint_t row) override
    {
      const MutableLine::Ptr res = MakePtr<MutableLineType>();
      Storage.Add(row, res);
      return *res;
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
    Pattern::Ptr Get(uint_t idx) const override
    {
      return Storage.Get(idx);
    }

    uint_t GetSize() const override
    {
      uint_t res = 0;
      for (uint_t idx = 0; idx != Storage.Size(); ++idx)
      {
        if (const Pattern::Ptr pat = Storage.Get(idx))
        {
          res += pat->GetSize() != 0;
        }
      }
      return res;
    }

    MutablePattern& AddPattern(uint_t idx) override
    {
      const MutablePattern::Ptr res = MakePtr<MutablePatternType>();
      Storage.Add(idx, res);
      return *res;
    }
  private:
    SparsedObjectsStorage<MutablePattern::Ptr> Storage;
  };

  Information::Ptr CreateTrackInfo(TrackModel::Ptr model, uint_t channels);

  class TrackStateIterator : public Iterator
  {
  public:
    typedef std::shared_ptr<TrackStateIterator> Ptr;

    virtual TrackModelState::Ptr GetStateObserver() const = 0;
  };

  TrackStateIterator::Ptr CreateTrackStateIterator(TrackModel::Ptr model);

  class PatternsBuilder : public Formats::Chiptune::PatternBuilder
  {
  public:
    explicit PatternsBuilder(MutablePatternsSet::Ptr patterns)
      : Patterns(std::move(patterns))
      , CurPattern()
      , CurLine()
      , CurChannel()
    {
    }

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
      CurLine = 0;
      CurChannel = 0;
    }

    void SetLine(uint_t idx)
    {
      CurLine = &CurPattern->AddLine(idx);
      CurChannel = 0;
    }

    void SetChannel(uint_t idx)
    {
      CurChannel = &CurLine->AddChannel(idx);
    }

    void FinishPattern(uint_t size)
    {
      CurPattern->SetSize(size);
      CurLine = 0;
      CurPattern = 0;
    }

    MutableLine& GetLine() const
    {
      return *CurLine;
    }

    MutableCell& GetChannel() const
    {
      return *CurChannel;
    }

    PatternsSet::Ptr GetResult() const
    {
      return Patterns;
    }

    template<uint_t ChannelsCount>
    static PatternsBuilder Create()
    {
      typedef MultichannelMutableLine<ChannelsCount> LineType;
      typedef SparsedMutablePattern<LineType> PatternType;
      typedef SparsedMutablePatternsSet<PatternType> PatternsSetType;
      return PatternsBuilder(MakePtr<PatternsSetType>());
    }
  private:  
    const MutablePatternsSet::Ptr Patterns;
    MutablePattern* CurPattern;
    MutableLine* CurLine;
    MutableCell* CurChannel;
  };
}
