/*
Abstract:
  Tracked modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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

namespace ZXTune
{
  namespace Module
  {
    // Ornament is just a set of tone offsets
    struct SimpleOrnament
    {
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

      virtual bool NextLine(Sound::LoopMode mode) = 0;

      virtual bool NextPosition(Sound::LoopMode mode) = 0;
    };

    // Basic template class for tracking support (used as simple parametrized namespace)
    template<uint_t ChannelsCount, class SampleType, class OrnamentType = SimpleOrnament>
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
        Command(uint_t type, int_t p1 = 0, int_t p2 = 0, int_t p3 = 0)
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

      struct Line
      {
        Line() : Tempo(), Channels()
        {
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

          bool FindCommand(uint_t type) const
          {
            return Commands.end() != std::find(Commands.begin(), Commands.end(), type);
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

      typedef std::vector<Line> Pattern;

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

        ModuleData() : Positions(), Patterns(), Samples(), Ornaments()
        {
        }

        virtual uint_t GetCurrentPattern(const TrackState& state) const
        {
          return Positions[state.Position()];
        }

        virtual uint_t GetCurrentPatternSize(const TrackState& state) const
        {
          return Patterns[GetCurrentPattern(state)].size();
        }

        virtual uint_t GetNewTempo(const TrackState& state) const
        {
          if (const boost::optional<uint_t>& tempo = Patterns[GetCurrentPattern(state)][state.Line()].Tempo)
          {
            return *tempo;
          }
          return 0;
        }

        void InitState(uint_t initTempo, uint_t totalFrames, Module::State& state) const
        {
          state = Module::State();
          Module::Tracking& trackState(state.Track);
          Module::Tracking& trackRef(state.Reference);
          trackRef.Position = Positions.size();
          trackState.Pattern = Positions[trackState.Position];
          trackRef.Pattern = Patterns.size();//use absolute index as a base
          trackRef.Line = Patterns[trackState.Pattern].size();
          if (const boost::optional<uint_t>& tempo = Patterns[trackState.Pattern][trackState.Line].Tempo)
          {
            trackRef.Quirk = *tempo;
          }
          else
          {
            trackRef.Quirk = initTempo;
          }
          trackRef.Frame = totalFrames;
          trackRef.Channels = ChannelsCount;
        }

        bool UpdateState(const Information& info, Sound::LoopMode loopMode, Module::State& state) const
        {
          //update tick outside of this func
          Module::Tracking& trackState(state.Track);
          Module::Tracking& trackRef(state.Reference);
          ++state.Frame;
          ++trackState.Frame;
          //check for next note
          if (++trackState.Quirk >= trackRef.Quirk)
          {
            trackState.Quirk = 0;
            //check for next position
            if (++trackState.Line >= trackRef.Line)
            {
              trackState.Line = 0;
              //check for finish
              if (++trackState.Position >= trackRef.Position)//end
              {
                //check if looped
                //do not reset ticks/frame in state!!!
                if (Sound::LOOP_NORMAL == loopMode)
                {
                  state.Frame = info.LoopFrame();
                  trackState.Position = info.LoopPosition();
                }
                else
                {
                  //keep possible tempo changes- do not call FillTiming
                  trackState.Frame = 0;
                  trackState.Position = 0;
                  if (Sound::LOOP_NONE == loopMode)
                  {
                    //exit here in initial position
                    return false;
                  }
                }
              }
              trackState.Pattern = Positions[trackState.Position];
              trackRef.Line = Patterns[trackState.Pattern].size();
            }  //pattern changed
            //update tempo if required
            if (const boost::optional<uint_t>& tempo = Patterns[trackState.Pattern][trackState.Line].Tempo)
            {
              trackRef.Quirk = *tempo;
            }
          } //line changed
          return true;
        }

        std::vector<uint_t> Positions;
        std::vector<Pattern> Patterns;
        std::vector<SampleType> Samples;
        std::vector<OrnamentType> Ornaments;
      };

      class ModuleInfo : public Information
      {
      public:
        explicit ModuleInfo(typename ModuleData::Ptr data)
          : Data(data)
          , LoopPosNum()
          , InitialTempo()
          , LogicChannels(ChannelsCount)
          , PhysChannels(ChannelsCount)
          , Frames(), LoopFrameNum()
        {
        }
        typedef boost::shared_ptr<ModuleInfo> Ptr;

        static Ptr Create(typename ModuleData::Ptr data)
        {
          return boost::make_shared<ModuleInfo>(data);
        }

        virtual uint_t PositionsCount() const
        {
          return Data->Positions.size();
        }

        virtual uint_t LoopPosition() const
        {
          return LoopPosNum;
        }

        virtual uint_t PatternsCount() const
        {
          return std::count_if(Data->Patterns.begin(), Data->Patterns.end(),
            !boost::bind(&Pattern::empty, _1));
        }

        virtual uint_t FramesCount() const
        {
          if (!Frames)
          {
            Initialize();
          }
          return Frames;
        }

        virtual uint_t LoopFrame() const
        {
          if (LoopPosNum && !LoopFrameNum)
          {
            Initialize();
          }
          return LoopFrameNum;
        }

        virtual uint_t LogicalChannels() const
        {
          return LogicChannels;
        }

        virtual uint_t PhysicalChannels() const
        {
          return PhysChannels;
        }

        virtual uint_t Tempo() const
        {
          return InitialTempo;
        }

        virtual Parameters::Accessor::Ptr Properties() const
        {
          assert(ModuleProperties);
          return ModuleProperties;
        }

        //modifiers
        void SetLoopPosition(uint_t loopPos)
        {
          LoopPosNum = loopPos;
        }

        void SetTempo(uint_t tempo)
        {
          InitialTempo = tempo;
        }

        void SetLogicalChannels(uint_t channels)
        {
          LogicChannels = channels;
        }

        void SetPhysicalChannels(uint_t channels)
        {
          PhysChannels = channels;
        }

        void SetModuleProperties(Parameters::Accessor::Ptr props)
        {
          ModuleProperties = props;
        }

      private:
        void Initialize() const
        {
          //emulate playback
          const Information::Ptr dummyInfo = boost::make_shared<ModuleInfo>(*this);
          const TrackStateIterator::Ptr dummyIterator = TrackStateIterator::Create(dummyInfo, Data, Analyzer::Ptr());

          TrackStateIterator& iterator = *dummyIterator;
          while (iterator.NextFrame(0, Sound::LOOP_NONE))
          {
            //check for loop
            if (0 == iterator.Line() &&
                0 == iterator.Quirk() &&
                LoopPosNum == iterator.Position())
            {
              LoopFrameNum = iterator.Frame();
            }
            //to prevent reset
            Frames = std::max(Frames, iterator.Frame());
          }
          ++Frames;
        }
      private:
        const typename ModuleData::Ptr Data;
        uint_t LoopPosNum;
        uint_t InitialTempo;
        uint_t LogicChannels;
        uint_t PhysChannels;
        Parameters::Accessor::Ptr ModuleProperties;
        mutable uint_t Frames;
        mutable uint_t LoopFrameNum;
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

    class StubTrackState : public TrackState
    {
    public:
      explicit StubTrackState(const Module::State& state)
          : State(state)
      {
      }

      //status functions
      virtual uint_t Position() const
      {
        return State.Track.Position;
      }

      virtual uint_t Pattern() const
      {
        return State.Track.Pattern;
      }

      virtual uint_t PatternSize() const
      {
        return State.Reference.Line;
      }

      virtual uint_t Line() const
      {
        return State.Track.Line;
      }

      virtual uint_t Tempo() const
      {
        return State.Reference.Quirk;
      }

      virtual uint_t Quirk() const
      {
        return State.Track.Quirk;
      }

      virtual uint_t Frame() const
      {
        return State.Track.Frame;
      }

      virtual uint_t Channels() const
      {
        return State.Track.Channels;
      }

      virtual uint_t AbsoluteFrame() const
      {
        return State.Frame;
      }

      virtual uint64_t AbsoluteTick() const
      {
        return State.Tick;
      }
    private:
      const Module::State& State;
    };

    class MergedTrackState : public TrackState
    {
    public:
      MergedTrackState(TrackState::Ptr first, TrackState::Ptr second)
        : First(first), Second(second)
      {
      }

      virtual uint_t Position() const
      {
        return First->Position();
      }

      virtual uint_t Pattern() const
      {
        return First->Pattern();
      }

      virtual uint_t PatternSize() const
      {
        return First->PatternSize();
      }

      virtual uint_t Line() const
      {
        return First->Line();
      }

      virtual uint_t Tempo() const
      {
        return First->Tempo();
      }

      virtual uint_t Quirk() const
      {
        return First->Quirk();
      }

      virtual uint_t Frame() const
      {
        return First->Frame();
      }

      virtual uint_t Channels() const
      {
        return First->Channels() + Second->Channels();
      }

      virtual uint_t AbsoluteFrame() const
      {
        return First->AbsoluteFrame();
      }

      virtual uint64_t AbsoluteTick() const
      {
        return First->AbsoluteTick();
      }
    private:
      const TrackState::Ptr First;
      const TrackState::Ptr Second;
    };

    class MergedAnalyzer : public Analyzer
    {
    public:
      MergedAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second)
        : First(first), Second(second)
      {
      }

      virtual uint_t ActiveChannels() const
      {
        return First->ActiveChannels() + Second->ActiveChannels();
      }

      virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
      {
        std::vector<std::pair<uint_t, uint_t> > firstLevels, secondLevels;
        First->BandLevels(firstLevels);
        Second->BandLevels(secondLevels);
        bandLevels.resize(firstLevels.size() + secondLevels.size());
        std::copy(secondLevels.begin(), secondLevels.end(),
          std::copy(firstLevels.begin(), firstLevels.end(), bandLevels.begin()));
      }
    private:
      const Analyzer::Ptr First;
      const Analyzer::Ptr Second;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
