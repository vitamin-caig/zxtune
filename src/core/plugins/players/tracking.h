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
//common includes
#include <messages_collector.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_types.h>
//std includes
#include <vector>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
//text includes
#include <core/text/warnings.h>

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
    template<uint_t ChannelsCount, class CommandType, class SampleType, class OrnamentType = SimpleOrnament>
    class TrackingSupport
    {
    public:
      // Define common types
      typedef SampleType Sample;
      typedef OrnamentType Ornament;

      struct Command
      {
        Command() : Type(), Param1(), Param2(), Param3()
        {
        }
        Command(CommandType type, int_t p1 = 0, int_t p2 = 0, int_t p3 = 0)
          : Type(type), Param1(p1), Param2(p2), Param3(p3)
        {
        }

        bool operator == (CommandType type) const
        {
          return Type == type;
        }

        CommandType Type;
        int_t Param1;
        int_t Param2;
        int_t Param3;
      };

      typedef std::vector<Command> CommandsArray;

      struct Line
      {
        Line() : Tempo(), Channels()
        {
        }

        void SetTempo(uint_t val)
        {
          Tempo = val;
        }

        //track attrs
        boost::optional<uint_t> Tempo;

        struct Chan
        {
          Chan() : Enabled(), Note(), SampleNum(), OrnamentNum(), Volume(), Commands()
          {
          }

          bool Empty() const
          {
            return !Enabled && !Note && !SampleNum && !OrnamentNum && !Volume && Commands.empty();
          }

          bool FindCommand(CommandType type) const
          {
            return Commands.end() != std::find(Commands.begin(), Commands.end(), type);
          }

          typedef bool(CommandsArray::*BoolType)() const;

          operator BoolType () const
          {
            return Empty() ? 0 : &CommandsArray::empty;
          }

          //modifiers
          void SetEnabled(bool val)
          {
            Enabled = val;
          }

          void SetNote(uint_t val)
          {
            Note = val;
          }

          void SetSample(uint_t val)
          {
            SampleNum = val;
          }

          void SetOrnament(uint_t val)
          {
            OrnamentNum = val;
          }

          void SetVolume(uint_t val)
          {
            Volume = val;
          }

          boost::optional<bool> Enabled;
          boost::optional<uint_t> Note;
          boost::optional<uint_t> SampleNum;
          boost::optional<uint_t> OrnamentNum;
          boost::optional<uint_t> Volume;
          CommandsArray Commands;
        };

        typedef boost::array<Chan, ChannelsCount> ChannelsArray;
        ChannelsArray Channels;
      };

      class Pattern
      {
      public:
        Pattern()
          : Size(0)
        {
        }

        Line& AddLine()
        {
          Lines.push_back(LineWithNumber(Size++));
          return Lines.back();
        }

        void AddLines(uint_t newLines)
        {
          Size += newLines;
        }

        const Line* GetLine(uint_t row) const
        {
          assert(row < Size);
          const typename LinesList::const_iterator it = std::lower_bound(Lines.begin(), Lines.end(), LineWithNumber(row));
          return it == Lines.end() || it->Number != row
            ? 0
            : &*it;
        }

        uint_t GetSize() const
        {
          return Size;
        }

        bool IsEmpty() const
        {
          return 0 == Size;
        }

        void Swap(Pattern& rh)
        {
          Lines.swap(rh.Lines);
          std::swap(Size, rh.Size);
        }
      private:
        struct LineWithNumber : public Line
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

      // Holder-related types
      class ModuleData : public TrackModuleData
      {
      public:
        typedef boost::shared_ptr<const ModuleData> Ptr;
        typedef boost::shared_ptr<ModuleData> RWPtr;

        static RWPtr Create()
        {
          return boost::make_shared<ModuleData>();
        }

        ModuleData()
          : LoopPosition(), InitialTempo()
          , Positions(), Patterns(), Samples(), Ornaments()
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
            !boost::bind(&Pattern::IsEmpty, _1)));
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
            if (const boost::optional<uint_t>& tempo = lineObj->Tempo)
            {
              return *tempo;
            }
          }
          return 0;
        }

        virtual uint_t GetActiveChannels(uint_t position, uint_t line) const
        {
          if (const Line* lineObj = Patterns[GetPatternIndex(position)].GetLine(line))
          {
            return static_cast<uint_t>(std::count_if(lineObj->Channels.begin(), lineObj->Channels.end(), !boost::bind(&Line::Chan::Empty, _1)));
          }
          return 0;
        }

        uint_t LoopPosition;
        uint_t InitialTempo;
        std::vector<uint_t> Positions;
        std::vector<Pattern> Patterns;
        std::vector<SampleType> Samples;
        std::vector<OrnamentType> Ornaments;
      };
    };

    //helper class to easy parse patterns
    struct PatternCursor
    {
      /*explicit*/PatternCursor(uint_t offset = 0)
        : Offset(offset), Period(), Counter()
      {
      }
      uint_t Offset;
      uint_t Period;
      uint_t Counter;

      void SkipLines(uint_t lines)
      {
        Counter -= lines;
      }

      static bool CompareByOffset(const PatternCursor& lh, const PatternCursor& rh)
      {
        return lh.Offset < rh.Offset;
      }

      static bool CompareByCounter(const PatternCursor& lh, const PatternCursor& rh)
      {
        return lh.Counter < rh.Counter;
      }
    };

    template<uint_t Channels>
    class PatternCursorSet : public boost::array<PatternCursor, Channels>
    {
      typedef boost::array<PatternCursor, Channels> Parent;
    public:
      uint_t GetMinCounter() const
      {
        return std::min_element(Parent::begin(), Parent::end(),
          Parent::value_type::CompareByCounter)->Counter;
      }

      uint_t GetMaxCounter() const
      {
        return std::max_element(Parent::begin(), Parent::end(),
          Parent::value_type::CompareByCounter)->Counter;
      }

      uint_t GetMaxOffset() const
      {
        return std::max_element(Parent::begin(), Parent::end(),
          Parent::value_type::CompareByOffset)->Offset;
      }

      void SkipLines(uint_t linesToSkip)
      {
        std::for_each(Parent::begin(), Parent::end(),
          std::bind2nd(std::mem_fun_ref(&Parent::value_type::SkipLines), linesToSkip));
      }
    };

  }
}

#endif //__CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
