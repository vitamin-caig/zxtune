/*
Abstract:
  AYLPT dumpers internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_BACKENDS_AYLPT_SUPP_DUMPER_H_DEFINED__
#define __SOUND_BACKENDS_AYLPT_SUPP_DUMPER_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <memory>

namespace ZXTune
{
  namespace AYM
  {
    struct DataChunk;
  }

  class AYLPTDumper
  {
  public:
    //protocol constants
    static const uint_t DATA_PORT = 0x378;
    static const uint_t CMD_PORT = 0x37a;
    static const uint_t INIT_CMD = 0;
    static const uint_t SELREG_CMD = 12;
    static const uint_t SETREG_CMD = 4;

    typedef std::auto_ptr<AYLPTDumper> Ptr;

    virtual ~AYLPTDumper() {}

    virtual void Process(const AYM::DataChunk& chunk) = 0;
  };

  //DLPortIO support
  namespace DLPortIO
  {
    bool IsSupported();
    AYLPTDumper::Ptr CreateDumper();
  }
}

#endif //__SOUND_BACKENDS_AYLPT_SUPP_DUMPER_H_DEFINED__
