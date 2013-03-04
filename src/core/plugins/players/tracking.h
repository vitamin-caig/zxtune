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
      virtual ~Line() {}

      virtual const Cell* GetChannel(uint_t idx) const = 0;
      virtual uint_t CountActiveChannels() const = 0;
      virtual uint_t GetTempo() const = 0;
    };

    class LineBuilder : public Line
    {
    public:
      virtual void SetTempo(uint_t val) = 0;
      virtual CellBuilder* AddChannel(uint_t idx) = 0;
    };

    class Pattern
    {
    public:
      virtual ~Pattern() {}

      virtual const Line* GetLine(uint_t row) const = 0;
      virtual uint_t GetSize() const = 0;
    };

    class PatternBuilder : public Pattern
    {
    public:
      virtual LineBuilder& AddLine() = 0;
      virtual void AddLines(uint_t skip) = 0;
    };

    template<uint_t ChannelsCount>
    class MultichannelLineBuilder : public LineBuilder
    {
    public:
      MultichannelLineBuilder()
        : Tempo()
      {
      }

      virtual const Cell* GetChannel(uint_t idx) const
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

      virtual CellBuilder* AddChannel(uint_t idx)
      {
        return &Channels[idx];
      }
    private:
      uint_t Tempo;
      typedef boost::array<CellBuilder, ChannelsCount> ChannelsArray;
      ChannelsArray Channels;
    };

    template<class LineBuilderType>
    class SparsedMultichannelPatternBuilder : public PatternBuilder
    {
    public:
      SparsedMultichannelPatternBuilder()
        : Size(0)
      {
      }

      virtual const Line* GetLine(uint_t row) const
      {
        if (row >= Size)
        {
          return 0;
        }
        const typename LinesList::const_iterator it = std::lower_bound(Lines.begin(), Lines.end(), LineWithNumber(row));
        return it == Lines.end() || it->Number != row
          ? 0
          : &*it;
      }

      virtual uint_t GetSize() const
      {
        return Size;
      }

      virtual LineBuilder& AddLine()
      {
        Lines.push_back(LineWithNumber(Size++));
        return Lines.back();
      }

      virtual void AddLines(uint_t newLines)
      {
        Size += newLines;
      }
    private:
      struct LineWithNumber : public LineBuilderType
      {
        LineWithNumber()
          : Number()
        {
        }

        explicit LineWithNumber(uint_t num)
          : Number(num)
        {
        }

        uint_t Number;

        bool operator < (const LineWithNumber& rh) const
        {
          return Number < rh.Number;
        }
      };
      typedef typename std::vector<LineWithNumber> LinesList;
      LinesList Lines;
      uint_t Size;
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
          return static_cast<uint_t>(std::count_if(Patterns.begin(), Patterns.end(),
            !boost::bind(&Pattern::GetSize, _1) != 0));
        }

        virtual uint_t GetPatternIndex(uint_t position) const
        {
          return Positions[position];
        }

        virtual uint_t GetPatternSize(uint_t position) const
        {
          return Patterns[GetPatternIndex(position)].GetSize();
        }

        virtual uint_t GetNewTempo(uint_t position, uint_t line) const
        {
          if (const Line* lineObj = Patterns[GetPatternIndex(position)].GetLine(line))
          {
            return lineObj->GetTempo();
          }
          return 0;
        }

        virtual uint_t GetActiveChannels(uint_t position, uint_t line) const
        {
          if (const Line* lineObj = Patterns[GetPatternIndex(position)].GetLine(line))
          {
            return lineObj->CountActiveChannels();
          }
          return 0;
        }

        uint_t LoopPosition;
        uint_t InitialTempo;
        std::vector<uint_t> Positions;
        std::vector<SparsedMultichannelPatternBuilder<MultichannelLineBuilder<ChannelsCount> > > Patterns;
      };

      struct BuildContext
      {
        ModuleData& Data;
        PatternBuilder* CurPattern;
        LineBuilder* CurLine;
        CellBuilder* CurChannel;

        explicit BuildContext(ModuleData& data)
          : Data(data)
          , CurPattern()
          , CurLine()
          , CurChannel()
        {
        }

        void SetPattern(uint_t idx)
        {
          Data.Patterns.resize(std::max<std::size_t>(idx + 1, Data.Patterns.size()));
          CurPattern = &Data.Patterns[idx];
          CurLine = 0;
          CurChannel = 0;
        }

        void SetLine(uint_t idx)
        {
          if (const uint_t skipped = idx - CurPattern->GetSize())
          {
            CurPattern->AddLines(skipped);
          }
          CurLine = &CurPattern->AddLine();
          CurChannel = 0;
        }

        void SetChannel(uint_t idx)
        {
          CurChannel = CurLine->AddChannel(idx);
        }

        void FinishPattern(uint_t size)
        {
          if (const uint_t skipped = size - CurPattern->GetSize())
          {
            CurPattern->AddLines(skipped);
          }
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
