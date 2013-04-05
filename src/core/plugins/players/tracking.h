/*
Abstract:
  Tracked modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__

//local includes
#include "iterator.h"
#include "track_model.h"
//library includes
#include <core/module_types.h>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace ZXTune
{
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
      virtual void SetTempo(uint_t val) = 0;
      virtual MutableCell& AddChannel(uint_t idx) = 0;
    };

    class MutablePattern : public Pattern
    {
    public:
      virtual MutableLine& AddLine(uint_t row) = 0;
      virtual void SetSize(uint_t size) = 0;
    };

    class MutablePatternsSet : public PatternsSet
    {
    public:
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

    //TODO: replace with boost::unordered_set
    template<class T>
    class SparsedObjectsStorage
    {
    public:
      SparsedObjectsStorage()
        : Count()
      {
      }

      T Get(uint_t idx) const
      {
        if (const T* res = Find(idx))
        {
          return *res;
        }
        else
        {
          return T();
        }
      }

      const T* Find(uint_t idx) const
      {
        if (idx >= Count)
        {
          return 0;
        }
        const typename ObjectsList::const_iterator it = std::lower_bound(Objects.begin(), Objects.end(), ObjectWithIndex(idx, T()));
        return it == Objects.end() || it->Index != idx
          ? 0
          : &it->Object;
      }

      uint_t Size() const
      {
        return Count;
      }

      void Resize(uint_t newSize)
      {
        assert(newSize >= Count);
        Count = newSize;
      }

      void Add(uint_t idx, T obj)
      {
        assert(Objects.end() == std::find(Objects.begin(), Objects.end(), idx));
        Objects.push_back(ObjectWithIndex(idx, obj));
        if (idx < Count)
        {
          std::sort(Objects.begin(), Objects.end());
        }
        else
        {
          Count = idx + 1;
        }
      }
    private:
      struct ObjectWithIndex
      {
        ObjectWithIndex()
          : Index()
          , Object()
        {
        }

        ObjectWithIndex(uint_t idx, T obj)
          : Index(idx)
          , Object(obj)
        {
        }

        uint_t Index;
        T Object;

        bool operator < (const ObjectWithIndex& rh) const
        {
          return Index < rh.Index;
        }

        bool operator == (uint_t idx) const
        {
          return Index == idx;
        }
      };
    private:
      uint_t Count;
      typedef std::vector<ObjectWithIndex> ObjectsList;
      ObjectsList Objects;
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
        const BuilderPtr res = boost::make_shared<MutableLineType>();
        Storage.Add(row, res);
        return *res;
      }

      virtual void SetSize(uint_t newSize)
      {
        Storage.Resize(newSize);
      }
    private:
      typedef boost::shared_ptr<MutableLineType> BuilderPtr;
      SparsedObjectsStorage<BuilderPtr> Storage;
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
        const BuilderPtr res = boost::make_shared<MutablePatternType>();
        Storage.Add(idx, res);
        return *res;
      }
    private:
      typedef boost::shared_ptr<MutablePattern> BuilderPtr;
      SparsedObjectsStorage<BuilderPtr> Storage;
    };

    Information::Ptr CreateTrackInfo(TrackModel::Ptr model, uint_t logicalChannels);
    Information::Ptr CreateTrackInfo(TrackModel::Ptr model, uint_t logicalChannels, uint_t physicalChannels);

    class TrackStateIterator : public Iterator
    {
    public:
      typedef boost::shared_ptr<TrackStateIterator> Ptr;

      virtual TrackModelState::Ptr GetStateObserver() const = 0;
    };

    TrackStateIterator::Ptr CreateTrackStateIterator(TrackModel::Ptr model);

    class PatternsBuilder
    {
    public:
      explicit PatternsBuilder(boost::shared_ptr<MutablePatternsSet> patterns)
        : Patterns(patterns)
        , CurPattern()
        , CurLine()
        , CurChannel()
      {
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

      PatternsSet::Ptr GetPatterns() const
      {
        return Patterns;
      }

      template<uint_t ChannelsCount>
      static PatternsBuilder Create()
      {
        typedef MultichannelMutableLine<ChannelsCount> LineType;
        typedef SparsedMutablePattern<LineType> PatternType;
        typedef SparsedMutablePatternsSet<PatternType> PatternsSetType;
        return PatternsBuilder(boost::make_shared<PatternsSetType>());
      }
    private:  
      const boost::shared_ptr<MutablePatternsSet> Patterns;
      MutablePattern* CurPattern;
      MutableLine* CurLine;
      MutableCell* CurChannel;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
