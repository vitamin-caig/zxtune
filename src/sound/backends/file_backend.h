/**
 *
 * @file
 *
 * @brief  File-based backends interfaces and fatory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/backends/backend_impl.h"

#include <binary/output_stream.h>
#include <sound/receiver.h>

namespace Sound
{
  namespace File
  {
    constexpr auto TITLE_TAG = "TITLE"sv;
    constexpr auto AUTHOR_TAG = "ARTIST"sv;
    constexpr auto COMMENT_TAG = "COMMENT"sv;
  }  // namespace File

  class FileStream : public Receiver
  {
  public:
    using Ptr = std::shared_ptr<FileStream>;

    virtual void SetTitle(const String& title) = 0;
    virtual void SetAuthor(const String& author) = 0;
    virtual void SetComment(const String& comment) = 0;
    virtual void FlushMetadata() = 0;
  };

  class FileStreamFactory
  {
  public:
    using Ptr = std::shared_ptr<const FileStreamFactory>;
    virtual ~FileStreamFactory() = default;

    virtual BackendId GetId() const = 0;
    virtual FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const = 0;
  };

  BackendWorker::Ptr CreateFileBackendWorker(Parameters::Accessor::Ptr params, Parameters::Accessor::Ptr properties,
                                             FileStreamFactory::Ptr factory);
}  // namespace Sound
