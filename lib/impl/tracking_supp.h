#ifndef __TRACKING_SUPP_H_DEFINED__
#define __TRACKING_SUPP_H_DEFINED__

#include <tools.h>

#include <sound_attrs.h>

#include <boost/array.hpp>

namespace ZXTune
{
  namespace Tracking
  {
    //code depends on channels count and sample type
    template<std::size_t ChannelsCount, class Sample>
    class TrackPlayer : public ModulePlayer
    {
    protected:
      //common structures
      /// Ornament is just a set of tone offsets
      struct Ornament
      {
        Ornament() : Loop()
        {
        }

        Ornament(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
        {

        }

        std::size_t Loop;
        std::vector<signed> Data;
      };

      struct Command
      {
        Command(unsigned type = 0, int p1 = 0, int p2 = 0) : Type(type), Param1(p1), Param2(p2)
        {
        }

        bool operator == (unsigned type) const
        {
          return Type == type;
        }

        unsigned Type;
        int Param1;
        int Param2;
      };

      typedef std::vector<Command> CommandsArray;

      struct Line
      {
        //track attrs
        Optional<std::size_t> Speed;

        struct Chan
        {
          Optional<bool> Enabled;
          Optional<std::size_t> Note;
          Optional<std::size_t> Sample;
          Optional<std::size_t> Ornament;
          Optional<std::size_t> Volume;
          CommandsArray Commands;
        };

        boost::array<Chan, ChannelsCount> Channels;
      };

      typedef std::vector<Line> Pattern;

      struct ModuleData
      {
        std::vector<std::size_t> Positions;
        std::vector<Pattern> Patterns;
        std::vector<Sample> Samples;
        std::vector<Ornament> Ornaments;
      };

    protected:
      //state
      struct ModuleState
      {
        Module::Tracking Position;
        uint32_t Frame;
        uint64_t Tick;
      };
    public:
      TrackPlayer() : PlaybackState(MODULE_STOPPED)
      {
      }

      /// Retrieving information about loaded module
      virtual void GetModuleInfo(Module::Information& info) const
      {
        info = Information;
      }

      /// Retrieving current state of loaded module
      virtual State GetModuleState(uint32_t& timeState, Module::Tracking& trackState) const
      {
        timeState = CurrentState.Frame;
        trackState = CurrentState.Position;
        return PlaybackState;
      }

      virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver* receiver)
      {
        if (++CurrentState.Position.Frame >= CurrentState.Position.Speed)//next note
        {
          CurrentState.Position.Frame = 0;
          if (++CurrentState.Position.Note >= Data.Patterns[CurrentState.Position.Pattern].size())//next position
          {
            CurrentState.Position.Note = 0;
            if (++CurrentState.Position.Position >= Information.Statistic.Position)//end
            {
              //set to loop
              if (params.Flags & Sound::MOD_LOOP)
              {
                CurrentState.Position.Pattern = Data.Positions[CurrentState.Position.Position = Information.Loop];
              }
              else
              {
                receiver->Flush();
                return PlaybackState = MODULE_STOPPED;
              }
            }
            CurrentState.Position.Pattern = Data.Positions[CurrentState.Position.Position];
          }
        }
        return PlaybackState = MODULE_PLAYING;
      }
      /// Controlling
      virtual State Reset()
      {
        CurrentState.Position = Module::Tracking();
        CurrentState.Position.Speed = Information.Statistic.Speed;
        CurrentState.Frame = 0;
        CurrentState.Tick = 0;
        CurrentState.Position.Pattern = Data.Positions[0];
        return PlaybackState = MODULE_STOPPED;
      }
    protected:
      Module::Information Information;
      ModuleData Data;

      ModuleState CurrentState;

      State PlaybackState;
    };
  }
}

#endif //__TRACKING_SUPP_H_DEFINED__
