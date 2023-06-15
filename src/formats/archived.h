/**
 *
 * @file
 *
 * @brief  Archived data support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/container.h>
#include <binary/format.h>
// std includes
#include <memory>

namespace Formats::Archived
{
  //! @brief Archived file presentation
  class File
  {
  public:
    using Ptr = std::shared_ptr<const File>;

    virtual ~File() = default;

    //! @brief Get archived file name
    //! @note In case of directories storing support, name will contain path separators
    virtual String GetName() const = 0;
    //! @brief Get archived file size
    //! @return Data size in bytes as stored in internal structures
    virtual std::size_t GetSize() const = 0;
    //! @brief Get archived file content
    //! @return Non-empty object if file is sucessfully extracted
    virtual Binary::Container::Ptr GetData() const = 0;
  };

  //! @brief Archive presentation
  //! @brief Parent Binary::Container presents original raw properties
  class Container : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    //! @brief DIP interface used to get all files inside archive
    class Walker
    {
    public:
      virtual ~Walker() = default;

      //! @brief Called on each visited file
      virtual void OnFile(const File& file) const = 0;
    };

    //! @brief Walks all the files stored in archive
    //! @note Order of walking is kept
    //! @param walker Reference to external files visitor
    virtual void ExploreFiles(const Walker& walker) const = 0;
    //! @brief Search archived file by name
    //! @param name Full file name as can be retrieved via File::GetName
    //! @return Non-null object if file is found
    virtual File::Ptr FindFile(StringView name) const = 0;
    //! @brief Count archived files
    //! @return Stored files count, may be 0
    virtual uint_t CountFiles() const = 0;
  };

  //! @brief Decoding functionality provider
  class Decoder
  {
  public:
    using Ptr = std::shared_ptr<const Decoder>;
    virtual ~Decoder() = default;

    //! @brief Get short decoder description
    virtual String GetDescription() const = 0;

    //! @brief Get approximate format description to search in raw binary data
    //! @invariant Cannot be empty
    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Perform raw data decoding
    //! @param rawData Data to be decoded
    //! @return Non-null object if data is successfully recognized and decoded
    //! @invariant Result is always rawData's subcontainer
    virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
  };
}  // namespace Formats::Archived
