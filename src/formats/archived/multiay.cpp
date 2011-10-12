/*
Abstract:
  AY containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core/plugins/utils.h"
//library includes
#include <formats/archived_decoders.h>
#include <formats/chiptune/ay.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace MultiAY
{
  class File : public Formats::Archived::File
  {
  public:
    File(const String& name, Binary::Container::Ptr data)
      : Name(name)
      , Data(data)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Data->Size();
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }
  private:
    const String Name;
    const Binary::Container::Ptr Data;
  };

  class Container : public Formats::Archived::Container
  {
  public:
    explicit Container(Binary::Container::Ptr data)
      : Delegate(data)
      , AyPath(Text::AY_FILENAME_PREFIX)
      , RawPath(Text::AY_RAW_FILENAME_PREFIX)
    {
    }

    //Binary::Container
    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual const void* Data() const
    {
      return Delegate->Data();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Container
    virtual void ExploreFiles(const Formats::Archived::Container::Walker& walker) const
    {
      for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
      {
        const Formats::Chiptune::AY::Builder::Ptr builder = Formats::Chiptune::AY::CreateFileBuilder();
        if (const Formats::Chiptune::Container::Ptr parsed = Formats::Chiptune::AY::Parse(*Delegate, idx, *builder))
        {
          const String subPath = AyPath.Build(idx);
          const Binary::Container::Ptr subData = builder->Result();
          const File file(subPath, subData);
          walker.OnFile(file);
        }
      }
    }

    virtual Formats::Archived::File::Ptr FindFile(const String& name) const
    {
      uint_t index = 0;
      const bool asRaw = RawPath.GetIndex(name, index);
      if (!asRaw && !AyPath.GetIndex(name, index))
      {
        return Formats::Archived::File::Ptr();
      }
      const uint_t subModules = Formats::Chiptune::AY::GetModulesCount(*Delegate);
      if (subModules < index)
      {
        return Formats::Archived::File::Ptr();
      }
      const Formats::Chiptune::AY::Builder::Ptr builder = asRaw
        ? Formats::Chiptune::AY::CreateMemoryDumpBuilder()
        : Formats::Chiptune::AY::CreateFileBuilder();
      if (!Formats::Chiptune::AY::Parse(*Delegate, index, *builder))
      {
        return Formats::Archived::File::Ptr();
      }
      const Binary::Container::Ptr data = builder->Result();
      return boost::make_shared<File>(name, data);
    }

    virtual uint_t CountFiles() const
    {
      return Formats::Chiptune::AY::GetModulesCount(*Delegate);
    }
  private:
    const Binary::Container::Ptr Delegate;
    const IndexPathComponent AyPath;
    const IndexPathComponent RawPath;
  };

  const std::string HEADER_FORMAT(
    "'Z'X'A'Y" // uint8_t Signature[4];
    "'E'M'U'L" // only one type is supported now
  );
}

namespace Formats
{
  namespace Archived
  {
    class MultiAYDecoder : public Formats::Archived::Decoder
    {
    public:
      MultiAYDecoder()
        : Format(Binary::Format::Create(MultiAY::HEADER_FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::AY_ARCHIVE_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Formats::Chiptune::AY::GetModulesCount(rawData) >= 2;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const uint_t subModules = Formats::Chiptune::AY::GetModulesCount(rawData);
        if (subModules < 2)
        {
          return Container::Ptr();
        }
        Formats::Chiptune::AY::Builder& stub = Formats::Chiptune::AY::GetStubBuilder();
        for (uint_t idx = subModules; idx; --idx)
        {
          if (Formats::Chiptune::Container::Ptr ayData = Formats::Chiptune::AY::Parse(rawData, idx - 1, stub))
          {
            return boost::make_shared< ::MultiAY::Container>(ayData);
          }
        }
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateAYDecoder()
    {
      return boost::make_shared<MultiAYDecoder>();
    }
  }
}
