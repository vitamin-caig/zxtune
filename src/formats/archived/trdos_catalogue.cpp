/**
 *
 * @file
 *
 * @brief  TR-DOS catalogue implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/trdos_catalogue.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/container_factories.h>
#include <binary/data_builder.h>
#include <strings/format.h>
// std includes
#include <cassert>
#include <cstring>
#include <numeric>
#include <utility>

namespace TRDos
{
  bool AreFilesMergeable(const File& lh, const File& rh)
  {
    const std::size_t firstSize = lh.GetSize();
    // merge if files are sequental
    if (lh.GetOffset() + firstSize != rh.GetOffset())
    {
      return false;
    }
    // and previous file size is multiplication of 255 sectors
    if (0 == (firstSize % 0xff00))
    {
      return true;
    }
    // xxx.x and
    // xxx    '.x should be merged
    static const Char SATTELITE_SEQ[] = {'\'', '.', '\0'};
    const String::size_type apPos(rh.GetName().find(SATTELITE_SEQ));
    const bool isSatelite = apPos != String::npos;
    if (!isSatelite)
    {
      return false;
    }
    const std::size_t secondSize = rh.GetSize();
    // ProDigiTracker
    if (firstSize == 0xc300 && secondSize == 0xc000)
    {
      return true;
    }
    // DigitalStudio
    if (firstSize == 0xf200 && secondSize == 0xc000)
    {
      return true;
    }
    // ExtremeTracker1
    if (firstSize == 0xfc00 && secondSize == 0xbc00)
    {
      return true;
    }
    return false;
  }

  using FilesList = std::vector<File::Ptr>;

  class MultiFile : public File
  {
  public:
    using Ptr = std::shared_ptr<MultiFile>;

    virtual bool Merge(File::Ptr other) = 0;

    virtual void SetContainer(Binary::Container::Ptr data) = 0;
  };

  class FixedNameFile : public File
  {
  public:
    FixedNameFile(StringView newName, File::Ptr delegate)
      : FixedName(newName.to_string())
      , Delegate(std::move(delegate))
    {}

    String GetName() const override
    {
      return FixedName;
    }

    std::size_t GetSize() const override
    {
      return Delegate->GetSize();
    }

    Binary::Container::Ptr GetData() const override
    {
      return Delegate->GetData();
    }

    std::size_t GetOffset() const override
    {
      return Delegate->GetOffset();
    }

  private:
    const String FixedName;
    const File::Ptr Delegate;
  };

  class NamesGenerator
  {
  public:
    explicit NamesGenerator(StringView name)
      : Name(name)
      , Idx(1)
    {}

    String operator()() const
    {
      return GenerateName();
    }

    void operator++()
    {
      ++Idx;
    }

  private:
    String GenerateName() const
    {
      assert(Idx);
      const auto dotPos = Name.find_last_of('.');
      const auto base = Name.substr(0, dotPos);
      const auto ext = dotPos == Name.npos ? StringView() : Name.substr(dotPos);
      return Strings::Format("{}~{}{}", base, Idx, ext);
    }

  private:
    const StringView Name;
    unsigned Idx;
  };

  class CommonCatalogue : public Binary::BaseContainer<Formats::Archived::Container>
  {
  public:
    template<class T>
    CommonCatalogue(const Binary::Container::Ptr& data, T from, T to)
      : BaseContainer(data)
      , Files(from, to)
    {}

    void ExploreFiles(const Formats::Archived::Container::Walker& walker) const override
    {
      for (const auto& file : Files)
      {
        walker.OnFile(*file);
      }
    }

    Formats::Archived::File::Ptr FindFile(StringView name) const override
    {
      for (const auto& file : Files)
      {
        if (file->GetName() == name)
        {
          return file;
        }
      }
      return {};
    }

    uint_t CountFiles() const override
    {
      return static_cast<uint_t>(Files.size());
    }

  private:
    const FilesList Files;
  };

  using MultiFilesList = std::vector<MultiFile::Ptr>;

  class BaseCatalogueBuilder : public CatalogueBuilder
  {
  public:
    void SetRawData(Binary::Container::Ptr data) override
    {
      for (auto& file : Files)
      {
        file->SetContainer(data);
      }
      Data = std::move(data);
    }

    void AddFile(File::Ptr newOne) override
    {
      if (!Merge(newOne))
      {
        AddNewOne(newOne);
      }
    }

    Formats::Archived::Container::Ptr GetResult() const override
    {
      if (Data && !Files.empty())
      {
        return MakePtr<CommonCatalogue>(Data, Files.begin(), Files.end());
      }
      else
      {
        return Formats::Archived::Container::Ptr();
      }
    }

  protected:
    virtual MultiFile::Ptr CreateMultiFile(File::Ptr inFile) = 0;

  private:
    bool Merge(File::Ptr newOne) const
    {
      return !Files.empty() && Files.back()->Merge(std::move(newOne));
    }

    void AddNewOne(File::Ptr newOne)
    {
      auto fixed = CreateUniqueNameFile(std::move(newOne));
      auto res = CreateMultiFile(std::move(fixed));
      res->SetContainer(Data);
      Files.emplace_back(std::move(res));
    }

    File::Ptr CreateUniqueNameFile(File::Ptr newOne)
    {
      const auto& originalName(newOne->GetName());
      if (!originalName.empty() && !HasFile(originalName))
      {
        return newOne;
      }
      for (NamesGenerator gen(originalName);; ++gen)
      {
        const auto newName = gen();
        if (!HasFile(newName))
        {
          return MakePtr<FixedNameFile>(newName, std::move(newOne));
        }
      }
    }

    bool HasFile(StringView name) const
    {
      for (const auto& file : Files)
      {
        if (file->GetName() == name)
        {
          return true;
        }
      }
      return false;
    }

  private:
    Binary::Container::Ptr Data;
    MultiFilesList Files;
  };

  // generic files support
  class GenericFile : public File
  {
  public:
    GenericFile(Binary::Container::Ptr data, StringView name, std::size_t off, std::size_t size)
      : Data(std::move(data))
      , Name(name.to_string())
      , Offset(off)
      , Size(size)
    {}

    String GetName() const override
    {
      return Name;
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      return Data;
    }

    std::size_t GetOffset() const override
    {
      return Offset;
    }

  private:
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
  };

  class GenericMultiFile : public MultiFile
  {
  public:
    explicit GenericMultiFile(File::Ptr delegate)
    {
      Subfiles.emplace_back(std::move(delegate));
    }

    String GetName() const override
    {
      return Subfiles.front()->GetName();
    }

    std::size_t GetSize() const override
    {
      return 1 == Subfiles.size()
                 ? Subfiles.front()->GetSize()
                 : std::accumulate(Subfiles.begin(), Subfiles.end(), std::size_t(0),
                                   [](std::size_t sum, const File::Ptr& file) { return sum + file->GetSize(); });
    }

    Binary::Container::Ptr GetData() const override
    {
      if (1 == Subfiles.size())
      {
        return Subfiles.front()->GetData();
      }
      Binary::DataBuilder res(GetSize());
      for (const auto& file : Subfiles)
      {
        const auto data = file->GetData();
        if (!data)
        {
          return {};
        }
        res.Add(*data);
      }
      return res.CaptureResult();
    }

    std::size_t GetOffset() const override
    {
      return Subfiles.front()->GetOffset();
    }

    bool Merge(File::Ptr rh) override
    {
      if (AreFilesMergeable(*Subfiles.back(), *rh))
      {
        Subfiles.emplace_back(std::move(rh));
        return true;
      }
      return false;
    }

    void SetContainer(Binary::Container::Ptr /*data*/) override {}

  private:
    FilesList Subfiles;
  };

  // TODO: remove implementation inheritance
  class GenericCatalogueBuilder : public BaseCatalogueBuilder
  {
  public:
    MultiFile::Ptr CreateMultiFile(File::Ptr inFile) override
    {
      return MakePtr<GenericMultiFile>(std::move(inFile));
    }
  };

  // flat files support
  class FlatFile : public File
  {
  public:
    FlatFile(StringView name, std::size_t off, std::size_t size)
      : Name(name.to_string())
      , Offset(off)
      , Size(size)
    {}

    String GetName() const override
    {
      return Name;
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      assert(!"Should not be called");
      return {};
    }

    std::size_t GetOffset() const override
    {
      return Offset;
    }

  private:
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
  };

  class FlatMultiFile : public MultiFile
  {
  public:
    FlatMultiFile(File::Ptr delegate)
      : Delegate(std::move(delegate))
      , Size(Delegate->GetSize())
    {}

    String GetName() const override
    {
      return Delegate->GetName();
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      return Data->GetSubcontainer(GetOffset(), Size);
    }

    std::size_t GetOffset() const override
    {
      return Delegate->GetOffset();
    }

    bool Merge(File::Ptr rh) override
    {
      if (AreFilesMergeable(*this, *rh))
      {
        Size += rh->GetSize();
        return true;
      }
      return false;
    }

    void SetContainer(Binary::Container::Ptr data) override
    {
      Data = std::move(data);
    }

  private:
    const File::Ptr Delegate;
    std::size_t Size;
    Binary::Container::Ptr Data;
  };

  // TODO: remove implementation inheritance
  class FlatCatalogueBuilder : public BaseCatalogueBuilder
  {
  public:
    MultiFile::Ptr CreateMultiFile(File::Ptr inFile) override
    {
      return MakePtr<FlatMultiFile>(std::move(inFile));
    }
  };

  File::Ptr File::Create(Binary::Container::Ptr data, StringView name, std::size_t off, std::size_t size)
  {
    return MakePtr<GenericFile>(std::move(data), name, off, size);
  }

  File::Ptr File::CreateReference(StringView name, std::size_t off, std::size_t size)
  {
    return MakePtr<FlatFile>(name, off, size);
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateGeneric()
  {
    return MakePtr<GenericCatalogueBuilder>();
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateFlat()
  {
    return MakePtr<FlatCatalogueBuilder>();
  }
}  // namespace TRDos
