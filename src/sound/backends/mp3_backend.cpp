/*
Abstract:
  MP3 file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef MP3_SUPPORT

//local includes
#include "enumerator.h"
#include "file_backend.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//platform-specific includes
#include <lame/lame.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 3B251603

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Mp3");

  const Char MP3_BACKEND_ID[] = {'m', 'p', '3', 0};

  struct LameLibraryTraits
  {
    static std::string GetName()
    {
      return "mp3lame";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<LameLibraryTraits> LameLibrary;

  typedef boost::shared_ptr<lame_global_flags> LameContextPtr;

  const std::size_t INITIAL_ENCODED_BUFFER_SIZE = 1048576;

  class Mp3Stream : public FileStream
  {
  public:
    Mp3Stream(uint_t soundFreq, LameContextPtr context, std::auto_ptr<std::ofstream> stream)
      : Stream(stream)
      , Context(context)
      , Encoded(INITIAL_ENCODED_BUFFER_SIZE)
    {
      ::lame_set_in_samplerate(Context.get(), soundFreq);
      ::lame_set_num_channels(Context.get(), OUTPUT_CHANNELS);
      ::lame_set_quality(Context.get(), 6);//TODO
      ::lame_init_params(Context.get());
      ::id3tag_init(Context.get());
      ::id3tag_add_v2(Context.get());
    }

    virtual ~Mp3Stream()
    {
      Flush();
    }

    virtual void SetTitle(const String& title)
    {
      const std::string titleC = IO::ConvertToFilename(title);
      ::id3tag_set_title(Context.get(), titleC.c_str());
    }

    virtual void SetAuthor(const String& author)
    {
      const std::string authorC = IO::ConvertToFilename(author);
      ::id3tag_set_artist(Context.get(), authorC.c_str());
    }

    virtual void SetComment(const String& comment)
    {
      const std::string commentC = IO::ConvertToFilename(comment);
      ::id3tag_set_comment(Context.get(), commentC.c_str());
    }

    virtual void StoreData(Chunk& data)
    {
      while (const int res = ::lame_encode_buffer_interleaved(Context.get(),
        safe_ptr_cast<short int*>(&data[0]), data.size(), &Encoded[0], Encoded.size()))
      {
        if (res > 0) //encoded
        {
          Stream->write(safe_ptr_cast<const char*>(&Encoded[0]), static_cast<std::streamsize>(res));
          break;
        }
        else if (-1 == res)//buffer too small
        {
          Encoded.resize(Encoded.size() * 2);
        }
        else if (-2 == res)//malloc problem
        {
          throw Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
        }
        else
        {
          throw Error(THIS_LINE, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
        }
      }
    }
  private:
    void Flush()
    {
      if (const int res = ::lame_encode_flush(Context.get(), &Encoded[0], Encoded.size()))
      {
        Stream->write(safe_ptr_cast<const char*>(&Encoded[0]), static_cast<std::streamsize>(res));
      }
    }
  private:
    const std::auto_ptr<std::ofstream> Stream;
    const LameContextPtr Context;
    Dump Encoded;
  };

  class Mp3FileFactory : public FileStreamFactory
  {
  public:
    explicit Mp3FileFactory(Parameters::Accessor::Ptr params)
      : RenderingParameters(RenderParameters::Create(params))
    {
    }

    virtual String GetId() const
    {
      return MP3_BACKEND_ID;
    }

    virtual FileStream::Ptr OpenStream(const String& fileName, bool overWrite) const
    {
      std::auto_ptr<std::ofstream> rawFile = IO::CreateFile(fileName, overWrite);
      const LameContextPtr context = LameContextPtr(::lame_init(), &::lame_close);
      return FileStream::Ptr(new Mp3Stream(RenderingParameters->SoundFreq(), context, rawFile));
    }
  private:
    const RenderParameters::Ptr RenderingParameters;
  };

  class MP3BackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return MP3_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::MP3_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const FileStreamFactory::Ptr factory = boost::make_shared<Mp3FileFactory>(allParams);
        const BackendWorker::Ptr worker = CreateFileBackendWorker(allParams, factory);
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterMP3Backend(BackendsEnumerator& enumerator)
    {
      if (LameLibrary::Instance().IsAccessible())
      {
        Log::Debug(THIS_MODULE, "Detected LAME library %1%", ::get_lame_version());
        const BackendCreator::Ptr creator(new MP3BackendCreator());
        enumerator.RegisterCreator(creator);
      }
    }
  }
}

//global namespace
#define STR(a) #a
#define LAME_CALL(func, ...) LameLibrary::Instance().GetSymbol(&func, STR(func))(__VA_ARGS__)

const char* get_lame_version()
{
  return LAME_CALL(get_lame_version);
}

lame_t lame_init()
{
  return LAME_CALL(lame_init);
}

int lame_close(lame_t ctx)
{
  return LAME_CALL(lame_close, ctx);
}

int lame_set_in_samplerate(lame_t ctx, int rate)
{
  return LAME_CALL(lame_set_in_samplerate, ctx, rate);
}

int lame_set_num_channels(lame_t ctx, int chans)
{
  return LAME_CALL(lame_set_num_channels, ctx, chans);
}

int lame_set_quality(lame_t ctx, int quality)
{
  return LAME_CALL(lame_set_quality, ctx, quality);
}

int lame_init_params(lame_t ctx)
{
  return LAME_CALL(lame_init_params, ctx);
}

int lame_encode_buffer_interleaved(lame_t ctx, short int* pcm, int samples, unsigned char* dst, int dstSize)
{
  return LAME_CALL(lame_encode_buffer_interleaved, ctx, pcm, samples, dst, dstSize);
}

int lame_encode_flush(lame_t ctx, unsigned char* dst, int dstSize)
{
  return LAME_CALL(lame_encode_flush, ctx, dst, dstSize);
}

void id3tag_init(lame_t ctx)
{
  return LAME_CALL(id3tag_init, ctx);
}

void id3tag_add_v2(lame_t ctx)
{
  return LAME_CALL(id3tag_add_v2, ctx);
}

void id3tag_set_title(lame_t ctx, const char* title)
{
  return LAME_CALL(id3tag_set_title, ctx, title);
}

void id3tag_set_artist(lame_t ctx, const char* artist)
{
  return LAME_CALL(id3tag_set_artist, ctx, artist);
}

void id3tag_set_comment(lame_t ctx, const char* comment)
{
  return LAME_CALL(id3tag_set_comment, ctx, comment);
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterMP3Backend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}
#endif
