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
#include <core/plugins/enumerator.h>
//common includes
#include <messages_collector.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_types.h>
#include <sound/render_params.h>// for LoopMode
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
        return Lines.size();
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
      virtual uint_t GetCurrentPattern(const TrackState& state) const = 0;
      virtual uint_t GetCurrentPatternSize(const TrackState& state) const = 0;
      virtual uint_t GetNewTempo(const TrackState& state) const = 0;
    };

    class TrackStateIterator : public TrackState
    {
    public:
      typedef boost::shared_ptr<TrackStateIterator> Ptr;

      static Ptr Create(Information::Ptr info, TrackModuleData::Ptr data, Analyzer::Ptr analyze);

      virtual void Reset() = 0;

      virtual void ResetPosition() = 0;

      virtual bool NextFrame(uint64_t ticksToSkip, Sound::LoopMode mode) = 0;
    };

    class TrackInfo : public Information
    {
    public:
      typedef boost::shared_ptr<TrackInfo> Ptr;

      virtual void SetLogicalChannels(uint_t channels) = 0;
      virtual void SetModuleProperties(Parameters::Accessor::Ptr props) = 0;

      static Ptr Create(TrackModuleData::Ptr data);
    };

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
          Lines.push_back(LineWithNumber(Size++, Line()));
          return Lines.back().second;
        }

        void AddLines(uint_t newLines)
        {
          Size += newLines;
        }

        const Line* GetLine(uint_t row) const
        {
          assert(row < Size);
          const typename LinesList::const_iterator it = std::lower_bound(Lines.begin(), Lines.end(), row,
            boost::bind(&LineWithNumber::first, _1) < _2);
          return it == Lines.end() || it->first != row
            ? 0
            : &it->second;
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
        typedef typename std::pair<uint_t, Line> LineWithNumber;
        typedef typename std::list<LineWithNumber> LinesList;
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
          return Positions.size();
        }

        virtual uint_t GetPatternsCount() const
        {
          return std::count_if(Patterns.begin(), Patterns.end(),
            !boost::bind(&Pattern::IsEmpty, _1));
        }

        virtual uint_t GetCurrentPattern(const TrackState& state) const
        {
          return Positions[state.Position()];
        }

        virtual uint_t GetCurrentPatternSize(const TrackState& state) const
        {
          return Patterns[GetCurrentPattern(state)].GetSize();
        }

        virtual uint_t GetNewTempo(const TrackState& state) const
        {
          if (const Line* line = Patterns[GetCurrentPattern(state)].GetLine(state.Line()))
          {
            if (const boost::optional<uint_t>& tempo = line->Tempo)
            {
              return *tempo;
            }
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
