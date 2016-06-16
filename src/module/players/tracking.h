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
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>

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
    typedef boost::shared_ptr<MutableLine> Ptr;
    
    virtual void SetTempo(uint_t val) = 0;
    virtual MutableCell& AddChannel(uint_t idx) = 0;
  };

  class MutablePattern : public Pattern
  {
  public:
    typedef boost::shared_ptr<MutablePattern> Ptr;
    
    virtual MutableLine& AddLine(uint_t row) = 0;
    virtual void SetSize(uint_t size) = 0;
  };

  class MutablePatternsSet : public PatternsSet
  {
  public:
    typedef boost::shared_ptr<MutablePatternsSet> Ptr;
    
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

    virtual Cell::Ptr GetChannel(uint_t idx) const
    {
      return Channels[idx].HasData() ? &Channels[idx] : 0;
    }

    virtual uint_t CountActiveChannels() const
    {
      return static_cast<uint_t>(std::count_if(Channels.begin(), Channels.end(), boost::bind(&Cell::HasData, _1)));
    }

    virtual uint_t GetTempo() const
    {
      return Tempo;
    }

    virtual void SetTempo(uint_t val)
    {
      Tempo = val;
    }

    virtual MutableCell& AddChannel(uint_t idx)
    {
      return Channels[idx];
    }
  private:
    uint_t Tempo;
    typedef boost::array<MutableCell, ChannelsCount> ChannelsArray;
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
    virtual Line::Ptr GetLine(uint_t row) const
    {
      return Storage.Get(row);
    }

    virtual uint_t GetSize() const
    {
      return Storage.Size();
    }

    virtual MutableLine& AddLine(uint_t row)
    {
      const MutableLine::Ptr res = MakePtr<MutableLineType>();
      Storage.Add(row, res);
      return *res;
    }

    virtual void SetSize(uint_t newSize)
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
    virtual Pattern::Ptr Get(uint_t idx) const
    {
      return Storage.Get(idx);
    }

    virtual uint_t GetSize() const
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

    virtual MutablePattern& AddPattern(uint_t idx)
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
    typedef boost::shared_ptr<TrackStateIterator> Ptr;

    virtual TrackModelState::Ptr GetStateObserver() const = 0;
  };

  TrackStateIterator::Ptr CreateTrackStateIterator(TrackModel::Ptr model);

  class PatternsBuilder : public Formats::Chiptune::PatternBuilder
  {
  public:
    explicit PatternsBuilder(MutablePatternsSet::Ptr patterns)
      : Patterns(patterns)
      , CurPattern()
      , CurLine()
      , CurChannel()
    {
    }

    virtual void Finish(uint_t size)
    {
      FinishPattern(size);
    }

    virtual void StartLine(uint_t index)
    {
      SetLine(index);
    }

    virtual void SetTempo(uint_t tempo)
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
