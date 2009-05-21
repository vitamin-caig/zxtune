#ifndef __TRACKING_SUPP_H_DEFINED__
#define __TRACKING_SUPP_H_DEFINED__

#include <tools.h>

#include <sound_attrs.h>

#include <boost/array.hpp>
#include <boost/optional.hpp>

namespace ZXTune
{
  namespace Tracking
  {
    //common structures
    /// Ornament is just a set of tone offsets
    struct SimpleOrnament
    {
      SimpleOrnament() : Loop(), Data()
      {
      }

      SimpleOrnament(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
      {

      }

      std::size_t Loop;
      std::vector<signed> Data;
    };

    //code depends on channels count and sample type
    template<std::size_t ChannelsCount, class SampleType, class OrnamentType = SimpleOrnament>
    class TrackPlayer : public ModulePlayer
    {
    protected:
      typedef SampleType Sample;
      typedef OrnamentType Ornament;
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
        Line() : Tempo(), Channels()
        {
        }
        //track attrs
        boost::optional<std::size_t> Tempo;

        struct Chan
        {
          Chan() : Enabled(), Note(), SampleNum(), OrnamentNum(), Volume(), Commands()
          {
          }

          bool Empty() const
          {
            return !Enabled && !Note && !SampleNum && !OrnamentNum && !Volume && Commands.empty();
          }

          boost::optional<bool> Enabled;
          boost::optional<std::size_t> Note;
          boost::optional<std::size_t> SampleNum;
          boost::optional<std::size_t> OrnamentNum;
          boost::optional<std::size_t> Volume;
          CommandsArray Commands;
        };

        boost::array<Chan, ChannelsCount> Channels;
      };

      typedef std::vector<Line> Pattern;

      struct ModuleData
      {
        ModuleData() : Positions(), Patterns(), Samples(), Ornaments()
        {
        }
        std::vector<std::size_t> Positions;
        std::vector<Pattern> Patterns;
        std::vector<Sample> Samples;
        std::vector<Ornament> Ornaments;
      };

    protected:
      //state
      struct ModuleState
      {
        ModuleState() : Position(), Frame(), Tick()
        {
        }
        Module::Tracking Position;
        std::size_t Frame;
        uint64_t Tick;
      };
    public:
      TrackPlayer() : Information(), Data(), CurrentState(), PlaybackState(MODULE_STOPPED)
      {
      }

      /// Retrieving information about loaded module
      virtual void GetModuleInfo(Module::Information& info) const
      {
        info = Information;
      }

      /// Retrieving current state of loaded module
      virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
      {
        timeState = CurrentState.Frame;
        trackState = CurrentState.Position;
        return PlaybackState;
      }

      virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
      {
        ++CurrentState.Frame;
        if (Navigate(CurrentState.Position))
        {
          return PlaybackState = MODULE_PLAYING;
        }
        else
        {
          //set to loop
          if (params.Flags & Sound::MOD_LOOP)
          {
            CurrentState.Position.Pattern = Data.Positions[CurrentState.Position.Position = Information.Loop];
            CurrentState.Frame = LoopFrame;
            UpdateTempo(CurrentState.Position);
            //do not reset ticks!
            return PlaybackState = MODULE_PLAYING;
          }
          else
          {
            receiver.Flush();
            return PlaybackState = MODULE_STOPPED;
          }
        }
      }
      /// Controlling
      virtual State Reset()
      {
        CurrentState = ModuleState();
        CurrentState.Position.Tempo = Information.Statistic.Tempo;
        CurrentState.Position.Pattern = Data.Positions[CurrentState.Position.Position];
        UpdateTempo(CurrentState.Position);
        return PlaybackState = MODULE_STOPPED;
      }
    private:
      bool Navigate(Module::Tracking& state)
      {
        if (++state.Frame >= state.Tempo)//next note
        {
          state.Frame = 0;
          if (++state.Note >= Data.Patterns[state.Pattern].size())//next position
          {
            state.Note = 0;
            if (++state.Position >= Data.Positions.size())//end
            {
              return false;
            }
            state.Pattern = Data.Positions[state.Position];
          }
          UpdateTempo(state);
        }
        return true;
      }

      void UpdateTempo(Module::Tracking& track)
      {
        assert(0 == track.Frame);
        const boost::optional<std::size_t>& tempo(Data.Patterns[track.Pattern][track.Note].Tempo);
        if (tempo)
        {
          track.Tempo = *tempo;
        }
      }
    protected:
      void InitTime()
      {
        Information.Statistic.Frame = 0;
        Reset();
        Module::Tracking& track(CurrentState.Position);
        do
        {
          if (0 == track.Frame)
          {
            UpdateTempo(track);
            if (0 == track.Note && track.Position == Information.Loop)
            {
              LoopFrame = Information.Statistic.Frame;
            }
          }
          ++Information.Statistic.Frame;
        }
        while (Navigate(track));
        Reset();
      }
    protected:
      Module::Information Information;
      ModuleData Data;

      ModuleState CurrentState;
      std::size_t LoopFrame;

      State PlaybackState;
    };
  }
}

#endif //__TRACKING_SUPP_H_DEFINED__
