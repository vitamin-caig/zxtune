/*
Abstract:
  File-based backends support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef SOUND_FILE_BACKEND_H_DEFINED
#define SOUND_FILE_BACKEND_H_DEFINED

//local includes
#include "backend_impl.h"
//library includes
#include <binary/output_stream.h>

namespace Sound
{
  typedef boost::shared_ptr<Chunk> ChunkPtr;

  typedef DataReceiver<ChunkPtr> ChunkStream;

  class FileStream : public ChunkStream
  {
  public:
    typedef boost::shared_ptr<FileStream> Ptr;

    virtual void SetTitle(const String& title) = 0;
    virtual void SetAuthor(const String& author) = 0;
    virtual void SetComment(const String& comment) = 0;
    virtual void FlushMetadata() = 0;
  };

  class FileStreamFactory
  {
  public:
    typedef boost::shared_ptr<const FileStreamFactory> Ptr;
    virtual ~FileStreamFactory() {}

    virtual String GetId() const = 0;
    virtual FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const = 0;
  };

  BackendWorker::Ptr CreateFileBackendWorker(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory);
}

#endif //SOUND_FILE_BACKEND_H_DEFINED
