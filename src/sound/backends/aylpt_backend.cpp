/*
Abstract:
  AYLPT backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#if defined(DLPORTIO_AYLPT_SUPPORT)

//local includes
#include "aylpt_supp/dumper.h"
#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"
//library includes
#include <core/convert_parameters.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <devices/aym.h>
#include <sound/backend_attrs.h>
#include <sound/error_codes.h>
//boost includes
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <sound/text/backends.h>

#define FILE_TAG C89925C1

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Char AYLPT_BACKEND_ID[] = {'a', 'y', 'l', 'p', 't', 0};
  const String AYLPT_BACKEND_VERSION(FromStdString("$Rev$"));

  class AYLPTBackend : public BackendImpl
                     , private boost::noncopyable
  {
  public:
    //TODO: parametrize
    explicit AYLPTBackend(Parameters::Accessor::Ptr soundParams)
      : BackendImpl(soundParams)
      , RenderPos(0)
      , Dumper(DLPortIO::CreateDumper())
      , Stub(MultichannelReceiver::CreateStub())
    {
    }

    Error SetModule(Module::Holder::Ptr holder)
    {
      if (holder.get())
      {
        const Plugin::Ptr plugin = holder->GetPlugin();
        const uint_t REQUIRED_CAPS = CAP_DEV_AYM | CAP_STOR_MODULE | CAP_CONV_ZX50;
        if (REQUIRED_CAPS != (plugin->Capabilities() & REQUIRED_CAPS))
        {
          return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_AYLPT_BACKEND_DUMP);
        }

        static const Module::Conversion::ZX50ConvertParam cnvParam;
        if (const Error& e = holder->Convert(cnvParam, RenderData))
        {
          return Error(THIS_LINE, BACKEND_SETUP_ERROR, Text::SOUND_ERROR_AYLPT_BACKEND_DUMP).AddSuberror(e);
        }
        RenderPos = 0;
      }
      return BackendImpl::SetModule(holder);
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      // volume control is not supported
      return VolumeControl::Ptr();
    }

    virtual void OnStartup()
    {
      OutputTime = boost::get_system_time();
    }

    virtual void OnShutdown()
    {
    }

    virtual void OnParametersChanged(const Parameters::Accessor&)
    {
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual bool OnRenderFrame()
    {
      Locker lock(PlayerMutex);
      //to update state
      Module::Player::PlaybackState state;
      ThrowIfError(Player->RenderFrame(*RenderingParameters, state, *Stub));
      return Module::Player::MODULE_PLAYING == state &&
             DoOutput();
    }

    virtual void OnBufferReady(std::vector<MultiSample>& /*buffer*/)
    {
    }
  private:
    bool DoOutput()
    {
      //prepare dump
      AYM::DataChunk chunk;
      {
        const std::size_t limit = RenderData.size();
        if (RenderPos + 2 > limit)
        {
          return false;
        }
        uint_t msk = RenderData[RenderPos + 1] * 256 + RenderData[RenderPos];
        RenderPos += 2;
        chunk.Mask = msk;
        if (RenderPos + CountBits(msk) > limit)
        {
          return false;
        }
        for (uint_t reg = 0; reg <= AYM::DataChunk::REG_ENV && msk; ++reg, msk >>= 1)
        {
          if (msk & 1)
          {
            chunk.Data[reg] = RenderData[RenderPos++];
          }
        }
      }
      //wait for next frame time
      const boost::system_time nextFrameTime = OutputTime + boost::posix_time::microseconds(RenderingParameters->FrameDurationMicrosec());
      {
        //lock mutex and wait for unlocking (which never happends) until frame ends
        boost::mutex localMutex;
        boost::unique_lock<boost::mutex> locker(localMutex);
        SyncEvent.timed_wait(locker, nextFrameTime);
      }
      //perform output
      OutputTime = boost::get_system_time();
      Dumper->Process(chunk);
      return true;
    }
  private:
    Dump RenderData;
    std::size_t RenderPos;
    AYLPTDumper::Ptr Dumper;
    const Sound::MultichannelReceiver::Ptr Stub;
    boost::system_time OutputTime;
    boost::condition_variable SyncEvent;
  };

  class AYLPTBackendCreator : public BackendCreator
                            , public boost::enable_shared_from_this<AYLPTBackendCreator>
  {
  public:
    virtual String Id() const
    {
      return AYLPT_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::AYLPT_BACKEND_DESCRIPTION;
    }

    virtual String Version() const
    {
      return AYLPT_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_HARDWARE;
    }

    virtual Error CreateBackend(Parameters::Accessor::Ptr params, Backend::Ptr& result) const
    {
      const BackendInformation::Ptr info = shared_from_this();
      return SafeBackendWrapper<AYLPTBackend>::Create(info, params, result, THIS_LINE);
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAYLPTBackend(BackendsEnumerator& enumerator)
    {
      if (DLPortIO::IsSupported())
      {
        const BackendCreator::Ptr creator(new AYLPTBackendCreator());
        enumerator.RegisterCreator(creator);
      }
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAYLPTBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
