/*
Abstract:
  File provider functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef IO_FILE_PROVIDER_H_DEFINED
#define IO_FILE_PROVIDER_H_DEFINED

//library includes
#include <binary/data.h>
#include <binary/output_stream.h>

namespace Parameters
{
  class Accessor;
}

namespace ZXTune
{
  namespace IO
  {
    Binary::Data::Ptr OpenLocalFile(const String& path, std::size_t mmapThreshold);

    class FileCreatingParameters
    {
    public:
      virtual ~FileCreatingParameters() {}

      virtual bool Overwrite() const = 0;
      virtual bool CreateDirectories() const = 0;
    };

    Binary::OutputStream::Ptr CreateLocalFile(const String& path, const FileCreatingParameters& params);
  }
}

#endif //IO_FILE_PROVIDER_H_DEFINED
