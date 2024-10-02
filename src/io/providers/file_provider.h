/**
 *
 * @file
 *
 * @brief  File provider functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <binary/data.h>
#include <binary/output_stream.h>

namespace Parameters
{
  class Accessor;
}

namespace IO
{
  Binary::Data::Ptr OpenLocalFile(StringView path, std::size_t mmapThreshold);

  enum class OverwriteMode
  {
    STOP_IF_EXISTS,
    OVERWRITE_EXISTING,
    RENAME_NEW
  };

  class FileCreatingParameters
  {
  public:
    virtual ~FileCreatingParameters() = default;

    virtual OverwriteMode Overwrite() const = 0;
    virtual bool CreateDirectories() const = 0;
    virtual bool SanitizeNames() const = 0;
  };

  Binary::SeekableOutputStream::Ptr CreateLocalFile(StringView path, const FileCreatingParameters& params);
}  // namespace IO
