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

//library includes
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
      struct ModuleData
      {
        typedef boost::shared_ptr<ModuleData> Ptr;
        typedef boost::shared_ptr<const ModuleData> ConstPtr;

        static Ptr Create()
        {
          return boost::make_shared<ModuleData>();
        }

        ModuleData() : Positions(), Patterns(), Samples(), Ornaments(), Info()
        {
        }

        void FillStatisticInfo(uint_t loopPosition, uint_t tempo, uint_t physicalChannels)
        {
          //static properties
          Info.PositionsCount = Positions.size();
          Info.LoopPosition = loopPosition;
          Info.PatternsCount = std::count_if(Patterns.begin(), Patterns.end(),
            !boost::bind(&Pattern::empty, _1));
          Info.Tempo = tempo;
          Info.LogicalChannels = ChannelsCount;
          Info.PhysicalChannels = physicalChannels;
          //emulate playback
          Module::State state;
          InitState(state);
          Module::Tracking& track = state.Track;
          do
          {
            //check for loop
            if (0 == track.Line &&
                0 == track.Quirk &&
                Info.LoopPosition == track.Position)
            {
              Info.LoopFrame = state.Frame;
            }
          }
          while (UpdateState(state, Sound::LOOP_NONE));
          Info.FramesCount = state.Frame;
        }

        void InitState(Module::State& state) const
        {
          state = Module::State();
          Module::Tracking& trackState(state.Track);
          Module::Tracking& trackRef(state.Reference);
          trackRef.Position = Info.PositionsCount;
          trackState.Pattern = Positions[trackState.Position];
          trackRef.Pattern = Patterns.size();//use absolute index as a base
          trackRef.Line = Patterns[trackState.Pattern].size();
          if (const boost::optional<uint_t>& tempo = Patterns[trackState.Pattern][trackState.Line].Tempo)
          {
            trackRef.Quirk = *tempo;
          }
          else
          {
            trackRef.Quirk = Info.Tempo;
          }
          trackRef.Frame = Info.FramesCount;
          trackRef.Channels = Info.LogicalChannels;
        }

        bool UpdateState(Module::State& state, Sound::LoopMode loopMode) const
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
                  state.Frame = Info.LoopFrame;
                  trackState.Position = Info.LoopPosition;
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
        Information Info;
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
  }
}

#endif //__CORE_PLUGINS_PLAYERS_TRACKING_H_DEFINED__
