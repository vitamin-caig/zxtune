/**
 *
 * @file
 *
 * @brief  TR-DOS catalogue helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <formats/archived.h>

namespace TRDos
{
  class File : public Formats::Archived::File
  {
  public:
    using Ptr = std::shared_ptr<const File>;

    virtual std::size_t GetOffset() const = 0;

    static Ptr Create(Binary::Container::Ptr data, StringView name, std::size_t off, std::size_t size);
    static Ptr CreateReference(StringView name, std::size_t off, std::size_t size);
  };

  class CatalogueBuilder
  {
  public:
    using Ptr = std::unique_ptr<CatalogueBuilder>;
    virtual ~CatalogueBuilder() = default;

    virtual void SetRawData(Binary::Container::Ptr data) = 0;
    virtual void AddFile(File::Ptr file) = 0;

    virtual Formats::Archived::Container::Ptr GetResult() const = 0;

    static Ptr CreateGeneric();
    static Ptr CreateFlat();
  };
}  // namespace TRDos
