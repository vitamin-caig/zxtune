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
#include <boost/optional.hpp>

namespace ZXTune
{
  namespace Module
  {
    // Ornament is just a set of tone offsets
    struct SimpleOrnament
    {
      SimpleOrnament() : Loop(), Data()
      {
      }

      SimpleOrnament(uint_t size, uint_t loop) : Loop(loop), Data(size)
      {
      }
      
      template<class It>
      SimpleOrnament(It from, It to, uint_t loop) : Loop(loop), Data(from, to)
      {
      }
      
      void Fix()
      {
        if (Data.empty())
        {
          Data.resize(1);
        }
      }

      uint_t Loop;
      std::vector<int_t> Data;
    };

    struct Timing
    {
      Timing() : Frame(), Tick()
      {
      }
      Module::Tracking Track;
      uint_t Frame;
      uint64_t Tick;
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
        ModuleData() : Positions(), Patterns(), Samples(), Ornaments(), Info()
        {
        }
        std::vector<uint_t> Positions;
        std::vector<Pattern> Patterns;
        std::vector<SampleType> Samples;
        std::vector<OrnamentType> Ornaments;
        Information Info;
      };

      //TODO: remove, for compartibility now
      typedef Module::Timing ModuleState;

      // Service functions
      static inline void CalculateTimings(const ModuleData& data, uint_t& framesCount, uint_t& loopFrame)
      {
        //emulate playback
        ModuleState state;
        InitState(data, state);
        Module::Tracking& track(state.Track);
        do
        {
          if (0 == track.Line &&
              0 == track.Frame &&
              data.Info.LoopPosition == track.Position)
          {
            loopFrame = state.Frame;
          }
        }
        while (UpdateState(data, state, Sound::LOOP_NONE));
        framesCount = state.Frame;
      }
       
      static inline void InitState(const ModuleData& data, ModuleState& state)
      {
        //reset state
        state = ModuleState();
        //set pattern to start
        state.Track.Pattern = data.Positions[state.Track.Position];
        //set tempo to start
        if (const boost::optional<uint_t>& tempo = data.Patterns[state.Track.Pattern][state.Track.Line].Tempo)
        {
          state.Track.Tempo = *tempo;
        }
        else
        {
          state.Track.Tempo = data.Info.Statistic.Tempo;
        }
      }
      
      static inline bool UpdateState(const ModuleData& data, ModuleState& state, Sound::LoopMode loopMode)
      {
        //update tick outside of this func
        ++state.Frame;
        Module::Tracking& trackState(state.Track);
        //check for next note
        if (++trackState.Frame >= trackState.Tempo)
        {
          trackState.Frame = 0;
          //check for next position
          if (++trackState.Line >= data.Patterns[trackState.Pattern].size())
          {
            trackState.Line = 0;
            //check for finish
            if (++trackState.Position >= data.Positions.size())//end
            {
              --trackState.Position;
              //check if looped
              //do not reset ticks in state!!!
              if (Sound::LOOP_NORMAL == loopMode)
              {
                state.Frame = data.Info.LoopFrame;
                trackState.Position = data.Info.LoopPosition;
              }
              else if (Sound::LOOP_BEGIN == loopMode)
              {
                state.Frame = 0;
                trackState.Position = 0;
              }
              else
              {
                return false;
              }
            }
            trackState.Pattern = data.Positions[trackState.Position];
          }
          //update tempo if required
          if (const boost::optional<uint_t>& tempo = data.Patterns[trackState.Pattern][trackState.Line].Tempo)
          {
            trackState.Tempo = *tempo;
          }
        }
        return true;
      }
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
