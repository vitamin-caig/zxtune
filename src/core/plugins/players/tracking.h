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

#include <core/module_types.h>
// for LoopMode
#include <sound/render_params.h>

#include <boost/array.hpp>
#include <boost/optional.hpp>

#include <vector>

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

      SimpleOrnament(unsigned size, unsigned loop) : Loop(loop), Data(size)
      {
      }
      
      template<class It>
      SimpleOrnament(It from, It to, unsigned loop) : Loop(loop), Data(from, to)
      {
      }
      
      void Fix()
      {
        if (Data.empty())
        {
          Data.resize(1);
        }
      }

      unsigned Loop;
      std::vector<signed> Data;
    };

    // Basic template class for tracking support (used as simple parametrized namespace)
    template<unsigned ChannelsCount, class SampleType, class OrnamentType = SimpleOrnament>
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
        Command(unsigned type, int p1 = 0, int p2 = 0, int p3 = 0)
          : Type(type), Param1(p1), Param2(p2), Param3(p3)
        {
        }

        bool operator == (unsigned type) const
        {
          return Type == type;
        }

        unsigned Type;
        int Param1;
        int Param2;
        int Param3;
      };

      typedef std::vector<Command> CommandsArray;

      struct Line
      {
        Line() : Tempo(), Channels()
        {
        }
        //track attrs
        boost::optional<unsigned> Tempo;

        struct Chan
        {
          Chan() : Enabled(), Note(), SampleNum(), OrnamentNum(), Volume(), Commands()
          {
          }

          bool Empty() const
          {
            return !Enabled && !Note && !SampleNum && !OrnamentNum && !Volume && Commands.empty();
          }

          bool FindCommand(unsigned type) const
          {
            return Commands.end() != std::find(Commands.begin(), Commands.end(), type);
          }

          boost::optional<bool> Enabled;
          boost::optional<unsigned> Note;
          boost::optional<unsigned> SampleNum;
          boost::optional<unsigned> OrnamentNum;
          boost::optional<unsigned> Volume;
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
        std::vector<unsigned> Positions;
        std::vector<Pattern> Patterns;
        std::vector<SampleType> Samples;
        std::vector<OrnamentType> Ornaments;
        Information Info;
      };

      // Player-related types
      struct ModuleState
      {
        ModuleState() : Frame(), Tick()
        {
        }
        Module::Tracking Track;
        unsigned Frame;
        uint64_t Tick;
      };
      
      // Service functions
      static inline void CalculateTimings(const ModuleData& data, unsigned& framesCount, unsigned& loopFrame)
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
        if (const boost::optional<unsigned>& tempo = data.Patterns[state.Track.Pattern][state.Track.Line].Tempo)
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
          if (const boost::optional<unsigned>& tempo = data.Patterns[trackState.Pattern][trackState.Line].Tempo)
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
      /*explicit*/PatternCursor(unsigned offset = 0)
        : Offset(offset), Period(), Counter()
      {
      }
      unsigned Offset;
      unsigned Period;
      unsigned Counter;

      void SkipLines(unsigned lines)
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
