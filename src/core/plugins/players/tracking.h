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
#include <core/plugins/enumerator.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_types.h>
//std includes
#include <vector>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace Module
  {
    // Ornament is just a set of tone offsets
    class SimpleOrnament
    {
    public:
      SimpleOrnament() : Loop(), Lines()
      {
      }

      template<class It>
      SimpleOrnament(uint_t loop, It from, It to) : Loop(loop), Lines(from, to)
      {
      }

      uint_t GetLoop() const
      {
        return Loop;
      }

      uint_t GetSize() const
      {
        return static_cast<uint_t>(Lines.size());
      }

      int_t GetLine(uint_t pos) const
      {
        return Lines.size() > pos ? Lines[pos] : 0;
      }

    private:
      uint_t Loop;
      std::vector<int_t> Lines;
    };

    struct Command
    {
      Command() : Type(), Param1(), Param2(), Param3()
      {
      }

      Command(uint_t type, int_t p1, int_t p2, int_t p3)
        : Type(type), Param1(p1), Param2(p2), Param3(p3)
      {
      }

      bool operator == (uint_t type) const
      {
        return Type == type;
      }

      uint_t Type;
      int_t Param1;
      int_t Param2;
      int_t Param3;
    };

    typedef std::vector<Command> CommandsArray;
    typedef RangeIterator<CommandsArray::const_iterator> CommandsIterator;

    class Cell
    {
    public:
      typedef const Cell* Ptr;

      Cell() : Mask(), Enabled(), Note(), SampleNum(), OrnamentNum(), Volume(), Commands()
      {
      }

      bool HasData() const
      {
        return 0 != Mask || !Commands.empty();
      }

      const bool* GetEnabled() const
      {
        return 0 != (Mask & ENABLED) ? &Enabled : 0;
      }

      const uint_t* GetNote() const
      {
        return 0 != (Mask & NOTE) ? &Note : 0;
      }

      const uint_t* GetSample() const
      {
        return 0 != (Mask & SAMPLENUM) ? &SampleNum : 0;
      }

      const uint_t* GetOrnament() const
      {
        return 0 != (Mask & ORNAMENTNUM) ? &OrnamentNum : 0;
      }

      const uint_t* GetVolume() const
      {
        return 0 != (Mask & VOLUME) ? &Volume : 0;
      }

      CommandsIterator GetCommands() const
      {
        return CommandsIterator(Commands.begin(), Commands.end());
      }
    protected:
      enum Flags
      {
        ENABLED = 1,
        NOTE = 2,
        SAMPLENUM = 4,
        ORNAMENTNUM = 8,
        VOLUME = 16
      };

      uint_t Mask;
      bool Enabled;
      uint_t Note;
      uint_t SampleNum;
      uint_t OrnamentNum;
      uint_t Volume;
      CommandsArray Commands;
    };

    class CellBuilder : public Cell
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

    class Line
    {
    public:
      typedef boost::shared_ptr<const Line> Ptr;
      virtual ~Line() {}

      virtual Cell::Ptr GetChannel(uint_t idx) const = 0;
      virtual uint_t CountActiveChannels() const = 0;
      virtual uint_t GetTempo() const = 0;
    };

    class LineBuilder : public Line
    {
    public:
      virtual void SetTempo(uint_t val) = 0;
      virtual CellBuilder& AddChannel(uint_t idx) = 0;
    };

    class Pattern
    {
    public:
      typedef boost::shared_ptr<const Pattern> Ptr;
      virtual ~Pattern() {}

      virtual Line::Ptr GetLine(uint_t row) const = 0;
      virtual uint_t GetSize() const = 0;
    };

    class PatternBuilder : public Pattern
    {
    public:
      virtual LineBuilder& AddLine(uint_t row) = 0;
      virtual void SetSize(uint_t size) = 0;
    };

    class PatternsSet
    {
    public:
      typedef boost::shared_ptr<const PatternsSet> Ptr;
      virtual ~PatternsSet() {}

      virtual Pattern::Ptr Get(uint_t idx) const = 0;
      virtual uint_t GetSize() const = 0;
    };

    class PatternsSetBuilder : public PatternsSet
    {
    public:
      virtual PatternBuilder& AddPattern(uint_t idx) = 0;
    };

    template<uint_t ChannelsCount>
    class MultichannelLineBuilder : public LineBuilder
    {
    public:
      MultichannelLineBuilder()
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
        assert(val != 0);
        Tempo = val;
      }

      virtual CellBuilder& AddChannel(uint_t idx)
      {
        return Channels[idx];
      }
    private:
      uint_t Tempo;
      typedef boost::array<CellBuilder, ChannelsCount> ChannelsArray;
      ChannelsArray Channels;
    };

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
        if (idx >= Count)
        {
          return T();
        }
        const typename ObjectsList::const_iterator it = std::lower_bound(Objects.begin(), Objects.end(), idx);
        return it == Objects.end() || it->Index != idx
          ? T()
          : it->Object;
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
        assert(idx >= Count);
        Objects.push_back(ObjectWithIndex(idx, obj));
        Count = idx + 1;
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

        bool operator < (uint_t idx) const
        {
          return Index < idx;
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

    template<class LineBuilderType>
    class SparsedPatternBuilder : public PatternBuilder
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

      virtual LineBuilder& AddLine(uint_t row)
      {
        const BuilderPtr res = boost::make_shared<LineBuilderType>();
        Storage.Add(row, res);
        return *res;
      }

      virtual void SetSize(uint_t newSize)
      {
        Storage.Resize(newSize);
      }
    private:
      typedef boost::shared_ptr<LineBuilderType> BuilderPtr;
      SparsedObjectsStorage<BuilderPtr> Storage;
    };

    template<class PatternBuilderType>
    class SparsedPatternsSetBuilder : public PatternsSetBuilder
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

      virtual PatternBuilder& AddPattern(uint_t idx)
      {
        const BuilderPtr res = boost::make_shared<PatternBuilderType>();
        Storage.Add(idx, res);
        return *res;
      }
    private:
      typedef boost::shared_ptr<PatternBuilder> BuilderPtr;
      SparsedObjectsStorage<BuilderPtr> Storage;
    };

    class TrackModuleData
    {
    public:
      typedef boost::shared_ptr<const TrackModuleData> Ptr;

      virtual ~TrackModuleData() {}

      //static
      virtual uint_t GetChannelsCount() const = 0;
      virtual uint_t GetLoopPosition() const = 0;
      virtual uint_t GetInitialTempo() const = 0;
      virtual uint_t GetPositionsCount() const = 0;
      virtual uint_t GetPatternsCount() const = 0;
      //dynamic
      virtual uint_t GetPatternIndex(uint_t position) const = 0;
      virtual uint_t GetPatternSize(uint_t position) const = 0;
      virtual uint_t GetNewTempo(uint_t position, uint_t line) const = 0;
      virtual uint_t GetActiveChannels(uint_t position, uint_t line) const = 0;
    };

    Information::Ptr CreateTrackInfo(TrackModuleData::Ptr data, uint_t logicalChannels);

    StateIterator::Ptr CreateTrackStateIterator(Information::Ptr info, TrackModuleData::Ptr data);

    // Basic template class for tracking support (used as simple parametrized namespace)
    template<uint_t ChannelsCount>
    class TrackingModel
    {
    public:
      static const uint_t CHANNELS = ChannelsCount;

      // Holder-related types
      class ModuleData : public TrackModuleData
      {
      public:
        ModuleData()
          : LoopPosition(), InitialTempo()
        {
        }

        virtual uint_t GetChannelsCount() const
        {
          return ChannelsCount;
        }

        virtual uint_t GetLoopPosition() const
        {
          return LoopPosition;
        }

        virtual uint_t GetInitialTempo() const
        {
          return InitialTempo;
        }

        virtual uint_t GetPositionsCount() const
        {
          return static_cast<uint_t>(Positions.size());
        }

        virtual uint_t GetPatternsCount() const
        {
          return Patterns->GetSize();
        }

        virtual uint_t GetPatternIndex(uint_t position) const
        {
          return Positions[position];
        }

        virtual uint_t GetPatternSize(uint_t position) const
        {
          return Patterns->Get(GetPatternIndex(position))->GetSize();
        }

        virtual uint_t GetNewTempo(uint_t position, uint_t line) const
        {
          if (const Line::Ptr lineObj = Patterns->Get(GetPatternIndex(position))->GetLine(line))
          {
            return lineObj->GetTempo();
          }
          return 0;
        }

        virtual uint_t GetActiveChannels(uint_t position, uint_t line) const
        {
          if (const Line::Ptr lineObj = Patterns->Get(GetPatternIndex(position))->GetLine(line))
          {
            return lineObj->CountActiveChannels();
          }
          return 0;
        }

        uint_t LoopPosition;
        uint_t InitialTempo;
        std::vector<uint_t> Positions;
        PatternsSet::Ptr Patterns;
      };

      struct BuildContext
      {
        typedef MultichannelLineBuilder<ChannelsCount> LineBuilderType;
        typedef SparsedPatternBuilder<LineBuilderType> PatternBuilderType;

        boost::shared_ptr<PatternsSetBuilder> Patterns;
        PatternBuilder* CurPattern;
        LineBuilder* CurLine;
        CellBuilder* CurChannel;

        explicit BuildContext(ModuleData& data)
          : Patterns(boost::make_shared<SparsedPatternsSetBuilder<PatternBuilderType> >())
          , CurPattern()
          , CurLine()
          , CurChannel()
        {
          data.Patterns = Patterns;
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
      };
    };

    template<uint_t ChannelsCount, class SampleType, class OrnamentType = SimpleOrnament>
    class TrackingSupport : public TrackingModel<ChannelsCount>
    {
    public:
      typedef SampleType Sample;
      typedef OrnamentType Ornament;

      class ModuleData : public TrackingModel<ChannelsCount>::ModuleData
      {
      public:
        typedef boost::shared_ptr<const ModuleData> Ptr;
        typedef boost::shared_ptr<ModuleData> RWPtr;

        static RWPtr Create()
        {
          return boost::make_shared<ModuleData>();
        }

        std::vector<SampleType> Samples;
        std::vector<OrnamentType> Ornaments;
      };
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
