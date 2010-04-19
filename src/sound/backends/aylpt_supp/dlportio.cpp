/*
Abstract:
  DLPortIO support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef DLPORTIO_AYLPT_SUPPORT

//local includes
#include "dumper.h"
#include <core/devices/aym.h>
//platform-dependent includes
#include <windows.h>

namespace
{
  using namespace ZXTune;

  //function prototype
  extern "C"
  {
    typedef void (*WriteByteFunc)(ULONG, UCHAR);
  }

  class Outputter
  {
  public:
    Outputter()
      : Library(::LoadLibrary("DLPORTIO.dll"))
      , Writer(reinterpret_cast<WriteByteFunc>(::GetProcAddress(Library, "DlPortWritePortUchar")))
    {
    }

    ~Outputter()
    {
      if (Library)
      {
        ::FreeLibrary(Library);
        Library = 0;
        Writer = 0;
      }
    }

    bool IsLoaded() const
    {
      return Library != 0 && Writer != 0;
    }

    void operator()(uint_t port, uint_t val)
    {
      assert(Writer);
      Writer(static_cast<ULONG>(port), static_cast<UCHAR>(val));
    }

  private:
    HMODULE Library;
    WriteByteFunc Writer;
  };

  class DLPortDumper : public AYLPTDumper
  {
  public:
    DLPortDumper()
    {
      assert(Out.IsLoaded());
      Out(CMD_PORT, INIT_CMD);
    }

    virtual void Process(const AYM::DataChunk& chunk)
    {
      for (uint_t msk = chunk.Mask, reg = 0; msk; msk >>= 1, ++reg)
      {
        if (msk & 1)
        {
          Out(DATA_PORT, reg);
          Out(CMD_PORT, SELREG_CMD);
          Out(DATA_PORT, chunk.Data[reg]);
          Out(CMD_PORT, SETREG_CMD);
          //boost::this_thread::sleep(boost::posix_time::milliseconds(10));//TODO: parametrize
        }
      }
    }
  private:
    Outputter Out;
  };
}

namespace ZXTune
{
  namespace DLPortIO
  {
    bool IsSupported()
    {
      Outputter out;
      return out.IsLoaded();
    }

    AYLPTDumper::Ptr CreateDumper()
    {
      return AYLPTDumper::Ptr(new DLPortDumper());
    }
  }
}

#else
//local includes
#include "dumper.h"
//std includes
#include <cassert>

namespace ZXTune
{
  namespace DLPortIO
  {
    bool IsSupported()
    {
      return false;
    }

    AYLPTDumper::Ptr CreateDumper()
    {
      assert(!"Never get here");
      return AYLPTDumper::Ptr(0);
    }
  }
}
#endif
