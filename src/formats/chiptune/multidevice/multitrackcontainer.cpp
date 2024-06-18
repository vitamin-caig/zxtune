/**
 *
 * @file
 *
 * @brief  Multitrack container support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/multidevice/multitrackcontainer.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/chiptune/container.h>
#include <math/numeric.h>
#include <strings/sanitize.h>
// std includes
#include <array>
#include <utility>

namespace IFF
{
  namespace Identifier
  {
    using Type = std::array<uint8_t, 4>;

    // Generic
    const Type AUTHOR = {{'A', 'U', 'T', 'H'}};
    const Type NAME = {{'N', 'A', 'M', 'E'}};
    const Type ANNOTATION = {{'A', 'N', 'N', 'O'}};

    // Custom
    const Type MTC1 = {{'M', 'T', 'C', '1'}};
    const Type TRACK = {{'T', 'R', 'C', 'K'}};
    const Type DATA = {{'D', 'A', 'T', 'A'}};
    const Type PROPERTY = {{'P', 'R', 'O', 'P'}};
  }  // namespace Identifier

  struct ChunkHeader
  {
    Identifier::Type Id;
    be_uint32_t DataSize;
  };

  static_assert(sizeof(ChunkHeader) * alignof(ChunkHeader) == 8, "Invalid layout");

  const std::size_t ALIGNMENT = 2;

  class Visitor
  {
  public:
    virtual ~Visitor() = default;

    virtual void OnChunk(const Identifier::Type& id, Binary::Container::Ptr content) = 0;
  };

  Binary::Container::Ptr Parse(const Binary::Container& input, Visitor& target)
  {
    Binary::InputStream stream(input);
    std::size_t pos = stream.GetPosition();
    for (;; pos = stream.GetPosition())
    {
      if (stream.GetRestSize() < sizeof(ChunkHeader))
      {
        break;
      }
      const auto& header = stream.Read<ChunkHeader>();
      const std::size_t dataSize = header.DataSize;
      if (stream.GetRestSize() < dataSize)
      {
        break;
      }
      auto data = input.GetSubcontainer(pos + sizeof(header), dataSize);
      target.OnChunk(header.Id, std::move(data));
      stream.Skip(Math::Align(dataSize, ALIGNMENT));
    }
    return input.GetSubcontainer(0, pos);
  }

  class ChunkSource
  {
  public:
    using Ptr = std::shared_ptr<const ChunkSource>;
    virtual ~ChunkSource() = default;

    virtual std::size_t GetSize() const = 0;
    virtual void GetResult(Binary::DataBuilder& builder) const = 0;
  };

  StringView GetStringView(Binary::View data)
  {
    return StringView(data.As<char>(), data.Size());
  }

  // Store in plain string, possibly UTF-8
  String GetString(Binary::View data)
  {
    return Strings::Sanitize(GetStringView(data));
  }

  class BlobChunkSourceBase : public ChunkSource
  {
  public:
    explicit BlobChunkSourceBase(Identifier::Type id)
      : Id(id)
    {}

    std::size_t GetSize() const override
    {
      return sizeof(ChunkHeader) + Math::Align(Size(), ALIGNMENT);
    }

    void GetResult(Binary::DataBuilder& builder) const override
    {
      const std::size_t size = Size();
      auto& hdr = builder.Add<ChunkHeader>();
      hdr.Id = Id;
      hdr.DataSize = size;
      if (size)
      {
        std::memcpy(builder.Allocate(Math::Align(size, ALIGNMENT)), Start(), size);
      }
    }

  protected:
    virtual std::size_t Size() const = 0;
    virtual const void* Start() const = 0;

  private:
    const Identifier::Type Id;
  };

  class DataChunkSource : public BlobChunkSourceBase
  {
  public:
    DataChunkSource(Identifier::Type id, Binary::Data::Ptr data)
      : BlobChunkSourceBase(id)
      , Data(std::move(data))
    {}

  protected:
    std::size_t Size() const override
    {
      return Data->Size();
    }

    const void* Start() const override
    {
      return Data->Start();
    }

  private:
    const Binary::Data::Ptr Data;
  };

  class StringChunkSource : public BlobChunkSourceBase
  {
  public:
    StringChunkSource(Identifier::Type id, StringView str)
      : BlobChunkSourceBase(id)
      , Data(str)
    {}

  protected:
    std::size_t Size() const override
    {
      return Data.size() * sizeof(Data[0]);
    }

    const void* Start() const override
    {
      return Data.data();
    }

  private:
    const String Data;
  };

  class CompositeChunkSource : public ChunkSource
  {
  public:
    explicit CompositeChunkSource(Identifier::Type id)
      : Id(id)
    {}

    void AddSubSource(ChunkSource::Ptr src)
    {
      const std::size_t subSize = src->GetSize();
      Sources.emplace_back(std::move(src));
      Require(0 == subSize % ALIGNMENT);
      TotalSize += subSize;
    }

    std::size_t GetSize() const override
    {
      return sizeof(ChunkHeader) + TotalSize;
    }

    void GetResult(Binary::DataBuilder& builder) const override
    {
      auto& hdr = builder.Add<ChunkHeader>();
      hdr.Id = Id;
      hdr.DataSize = TotalSize;
      for (const auto& src : Sources)
      {
        src->GetResult(builder);
      }
    }

  private:
    const Identifier::Type Id;
    std::vector<ChunkSource::Ptr> Sources;
    std::size_t TotalSize = 0;
  };

  class DataBuilder
    : public Visitor
    , public CompositeChunkSource
  {
  public:
    using Ptr = std::shared_ptr<DataBuilder>;

    explicit DataBuilder(const Identifier::Type& id)
      : CompositeChunkSource(id)
    {}

    void OnString(const Identifier::Type& id, StringView str)
    {
      AddSubSource(MakePtr<StringChunkSource>(id, str));
    }

    void OnChunk(const Identifier::Type& id, Binary::Container::Ptr content) override
    {
      AddSubSource(MakePtr<DataChunkSource>(id, std::move(content)));
    }
  };
}  // namespace IFF

namespace Formats::Chiptune
{
  namespace MultiTrackContainer
  {
    const Char DESCRIPTION[] = "Multitrack Container";

    class StubBuilder : public Builder
    {
    public:
      void SetAuthor(StringView /*author*/) override {}
      void SetTitle(StringView /*title*/) override {}
      void SetAnnotation(StringView /*annotation*/) override {}
      void SetProperty(StringView /*name*/, StringView /*value*/) override {}

      void StartTrack(uint_t /*idx*/) override {}
      void SetData(Binary::Container::Ptr /*data*/) override {}
    };

    bool FastCheck(Binary::View rawData)
    {
      const auto* header = rawData.As<IFF::ChunkHeader>();
      return header && header->Id == IFF::Identifier::MTC1;
    }

    const std::size_t MIN_SIZE = sizeof(IFF::ChunkHeader) * 3 + 256;

    const auto FORMAT =
        "'M'T'C'1"
        "00 00-10 ? ?"  // max 1Mb
        ""_sv;

    const auto PROPERTY_DELIMITER = "="_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {}

      String GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return FastCheck(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

    class BlobBuilder : public ContainerBuilder
    {
    public:
      BlobBuilder()
        : Tune(MakePtr<IFF::DataBuilder>(IFF::Identifier::MTC1))
        , Context(Tune)
      {}

      void SetAuthor(StringView author) override
      {
        if (!author.empty())
        {
          Context->OnString(IFF::Identifier::AUTHOR, author);
        }
      }

      void SetTitle(StringView title) override
      {
        if (!title.empty())
        {
          Context->OnString(IFF::Identifier::NAME, title);
        }
      }

      void SetAnnotation(StringView annotation) override
      {
        if (!annotation.empty())
        {
          Context->OnString(IFF::Identifier::ANNOTATION, annotation);
        }
      }

      void SetProperty(StringView name, StringView value) override
      {
        Require(name.npos == name.find_first_of(PROPERTY_DELIMITER));
        if (!value.empty())
        {
          // TODO: make Concat(StringView str...)
          Context->OnString(IFF::Identifier::PROPERTY, String{name} + PROPERTY_DELIMITER + value);
        }
        else
        {
          Context->OnString(IFF::Identifier::PROPERTY, name);
        }
      }

      void StartTrack(uint_t /*idx*/) override
      {
        FinishTrack();
        Track = MakePtr<IFF::DataBuilder>(IFF::Identifier::TRACK);
        Context = Track;
      }

      void SetData(Binary::Container::Ptr data) override
      {
        Require(Context == Track);
        Track->OnChunk(IFF::Identifier::DATA, std::move(data));
      }

      Binary::Data::Ptr GetResult() override
      {
        FinishTrack();
        Binary::DataBuilder builder(Tune->GetSize());
        Tune->GetResult(builder);
        return builder.CaptureResult();
      }

    private:
      void FinishTrack()
      {
        if (Track)
        {
          Tune->AddSubSource(Track);
          Track = IFF::DataBuilder::Ptr();
          Context = Tune;
        }
      }

    private:
      IFF::DataBuilder::Ptr Tune;
      IFF::DataBuilder::Ptr Track;
      IFF::DataBuilder::Ptr Context;
    };

    ContainerBuilder::Ptr CreateBuilder()
    {
      return MakePtr<BlobBuilder>();
    }

    class MetadataParser : public IFF::Visitor
    {
    public:
      explicit MetadataParser(Builder& delegate)
        : Delegate(delegate)
      {}

      void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content) override
      {
        if (!content)
        {
          return;
        }
        else if (id == IFF::Identifier::AUTHOR)
        {
          Delegate.SetAuthor(IFF::GetString(*content));
        }
        else if (id == IFF::Identifier::NAME)
        {
          Delegate.SetTitle(IFF::GetString(*content));
        }
        else if (id == IFF::Identifier::ANNOTATION)
        {
          Delegate.SetAnnotation(IFF::GetString(*content));
        }
        else if (id == IFF::Identifier::PROPERTY)
        {
          const auto property = IFF::GetStringView(*content);
          const auto eqPos = property.find_first_of(PROPERTY_DELIMITER);
          const auto name = property.substr(0, eqPos);
          const auto value = eqPos != String::npos ? property.substr(eqPos + 1) : StringView();
          Delegate.SetProperty(name, value);
        }
      }

    private:
      Builder& Delegate;
    };

    class TrackParser : public IFF::Visitor
    {
    public:
      TrackParser(Builder& delegate)
        : Delegate(delegate)
        , Metadata(delegate)
      {}

      void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content) override
      {
        if (id == IFF::Identifier::DATA)
        {
          Require(!!content);
          Delegate.SetData(std::move(content));
        }
        else
        {
          Metadata.OnChunk(id, std::move(content));
        }
      }

    private:
      Builder& Delegate;
      MetadataParser Metadata;
    };

    class TuneParser : public IFF::Visitor
    {
    public:
      explicit TuneParser(Builder& delegate)
        : Delegate(delegate)
        , Metadata(delegate)
      {}

      void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content) override
      {
        if (id == IFF::Identifier::TRACK)
        {
          Require(!!content);
          ParseTrack(*content);
        }
        else
        {
          Metadata.OnChunk(id, std::move(content));
        }
      }

    private:
      void ParseTrack(const Binary::Container& content)
      {
        Delegate.StartTrack(CurTrack++);
        TrackParser track(Delegate);
        IFF::Parse(content, track);
      }

    private:
      Builder& Delegate;
      MetadataParser Metadata;
      uint_t CurTrack = 0;
    };

    class FileParser : public IFF::Visitor
    {
    public:
      explicit FileParser(Builder& delegate)
        : Delegate(delegate)
      {}

      void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content) override
      {
        Require(id == IFF::Identifier::MTC1);
        Require(!!content);
        ParseTune(*content);
      }

    private:
      void ParseTune(const Binary::Container& content)
      {
        Require(CurTune == 0);  // only one tune is allowed
        ++CurTune;
        TuneParser tune(Delegate);
        IFF::Parse(content, tune);
      }

    private:
      Builder& Delegate;
      uint_t CurTune = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return {};
      }
      try
      {
        FileParser file(target);
        if (auto result = IFF::Parse(data, file))
        {
          const auto totalSize = result->Size();
          return CreateCalculatingCrcContainer(std::move(result), 0, totalSize);
        }
      }
      catch (const std::exception& /*e*/)
      {}
      return {};
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }  // namespace MultiTrackContainer

  Decoder::Ptr CreateMultiTrackContainerDecoder()
  {
    return MakePtr<MultiTrackContainer::Decoder>();
  }
}  // namespace Formats::Chiptune
