/**
* 
* @file
*
* @brief  Multitrack container support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multitrackcontainer.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/data_builder.h>
#include <binary/input_stream.h>
#include <binary/format_factories.h>
#include <formats/chiptune/container.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace IFF
{
  namespace Identifier
  {
    typedef boost::array<uint8_t, 4> Type;
    
    //Generic
    const Type AUTHOR = {{'A', 'U', 'T', 'H'}};
    const Type NAME = {{'N', 'A', 'M', 'E'}};
    const Type ANNOTATION = {{'A', 'N', 'N', 'O'}};
    const Type VERSION = {{'F', 'V', 'E', 'R'}};
    
    //Custom
    const Type MTC1 = {{'M', 'T', 'C', '1'}};
    const Type TRACK = {{'T', 'R', 'C', 'K'}};
    const Type DATA = {{'D', 'A', 'T', 'A'}};
    const Type PROPERTY = {{'P', 'R', 'O', 'P'}};
  }  

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct ChunkHeader
  {
    Identifier::Type Id;
    uint32_t DataSize;
  } PACK_POST;
  
  const std::size_t ALIGNMENT = 2;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  class Visitor
  {
  public:
    virtual ~Visitor() {}
    
    virtual void OnChunk(const Identifier::Type& id, Binary::Container::Ptr content) = 0;
  };
  
  Binary::Container::Ptr Parse(const Binary::Container& input, Visitor& target)
  {
    Binary::InputStream stream(input);
    std::size_t pos = stream.GetPosition();
    for (; ; pos = stream.GetPosition())
    {
      if (stream.GetRestSize() < sizeof(ChunkHeader))
      {
        break;
      }
      const ChunkHeader& header = stream.ReadField<ChunkHeader>();
      const std::size_t dataSize = fromBE(header.DataSize);
      if (stream.GetRestSize() < dataSize)
      {
        break;
      }
      const Binary::Container::Ptr data = input.GetSubcontainer(pos + sizeof(header), dataSize);
      target.OnChunk(header.Id, data);
      stream.ReadData(Math::Align(dataSize, ALIGNMENT));
    }
    return input.GetSubcontainer(0, pos);
  }
  
  class ChunkSource
  {
  public:
    typedef boost::shared_ptr<const ChunkSource> Ptr;
    virtual ~ChunkSource() {}
    
    virtual std::size_t GetSize() const = 0;
    virtual void GetResult(Binary::DataBuilder& builder) const = 0;
  };
  
  //Store in plain string, possibly UTF-8
  std::string GetString(const Binary::Data& data)
  {
    const char* const str = static_cast<const char*>(data.Start());
    const std::size_t count = data.Size();
    return std::string(str, str + count);
  }
  
  class BlobChunkSourceBase : public ChunkSource
  {
  public:
    explicit BlobChunkSourceBase(const Identifier::Type& id)
      : Id(id)
    {
    }
    
    virtual std::size_t GetSize() const
    {
      return sizeof(ChunkHeader) + Math::Align(Size(), ALIGNMENT);
    }

    virtual void GetResult(Binary::DataBuilder& builder) const
    {
      const std::size_t size = Size();
      ChunkHeader& hdr = builder.Add<ChunkHeader>();
      hdr.Id = Id;
      hdr.DataSize = fromBE<uint32_t>(size);
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
    DataChunkSource(const Identifier::Type& id, Binary::Data::Ptr data)
      : BlobChunkSourceBase(id)
      , Data(data)
    {
    }
  protected:
    virtual std::size_t Size() const
    {
      return Data->Size();
    }
    
    virtual const void* Start() const
    {
      return Data->Start();
    }
  private:
    const Binary::Data::Ptr Data;
  };
  
  class StringChunkSource : public BlobChunkSourceBase
  {
  public:
    StringChunkSource(const Identifier::Type& id, const String& str)
      : BlobChunkSourceBase(id)
      , Data(str)
    {
    }
  protected:
    virtual std::size_t Size() const
    {
      return Data.size() * sizeof(Data[0]);
    }
    
    virtual const void* Start() const
    {
      return Data.data();
    }
  private:
    const String Data;
  };
  
  class CompositeChunkSource : public ChunkSource
  {
  public:
    explicit CompositeChunkSource(const Identifier::Type& id)
      : Id(id)
      , TotalSize()
    {
    }
    
    void AddSubSource(ChunkSource::Ptr src)
    {
      Sources.push_back(src);
      const std::size_t subSize = src->GetSize();
      Require(0 == subSize % ALIGNMENT);
      TotalSize += subSize;
    }
    
    virtual std::size_t GetSize() const
    {
      return sizeof(ChunkHeader) + TotalSize;
    }
    
    virtual void GetResult(Binary::DataBuilder& builder) const
    {
      ChunkHeader& hdr = builder.Add<ChunkHeader>();
      hdr.Id = Id;
      hdr.DataSize = fromBE<uint32_t>(TotalSize);
      for (std::vector<ChunkSource::Ptr>::const_iterator it = Sources.begin(), lim = Sources.end(); it != lim; ++it)
      {
        (*it)->GetResult(builder);
      }
    }
  private:
    const Identifier::Type Id;
    std::vector<ChunkSource::Ptr> Sources;
    std::size_t TotalSize;
  };
  
  class DataBuilder : public Visitor, public CompositeChunkSource
  {
  public:
    typedef boost::shared_ptr<DataBuilder> Ptr;
    
    explicit DataBuilder(const Identifier::Type& id)
      : CompositeChunkSource(id)
    {
    }
    
    void OnString(const Identifier::Type& id, const String& str)
    {
      AddSubSource(boost::make_shared<StringChunkSource>(id, str));
    }
    
    virtual void OnChunk(const Identifier::Type& id, Binary::Container::Ptr content)
    {
      AddSubSource(boost::make_shared<DataChunkSource>(id, content));
    }
  };
}

namespace Formats
{
namespace Chiptune
{
  namespace MultiTrackContainer
  {
    class StubBuilder : public Builder
    {
    public:
      virtual void SetAuthor(const String& /*author*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAnnotation(const String& /*annotation*/) {}
      virtual void SetProperty(const String& /*property*/) {}

      virtual void StartTrack(uint_t /*idx*/) {}
      virtual void SetData(Binary::Container::Ptr /*data*/) {}
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      if (rawData.Size() <= sizeof(IFF::ChunkHeader))
      {
        return false;
      }
      const IFF::ChunkHeader& header = *static_cast<const IFF::ChunkHeader*>(rawData.Start());
      return header.Id == IFF::Identifier::MTC1;
    }

    const std::size_t MIN_SIZE = sizeof(IFF::ChunkHeader) * 3 + 256;
    
    const std::string FORMAT(
      "'M'T'C'1"
      "00 00-10 ? ?" //max 1Mb
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::MULTITRACK_CONTAINER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
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
        : Tune(boost::make_shared<IFF::DataBuilder>(IFF::Identifier::MTC1))
        , Context(Tune)
      {
      }
      
      virtual void SetAuthor(const String& author)
      {
        if (!author.empty())
        {
          Context->OnString(IFF::Identifier::AUTHOR, author);
        }
      }
      
      virtual void SetTitle(const String& title)
      {
        if (!title.empty())
        {
          Context->OnString(IFF::Identifier::NAME, title);
        }
      }
      
      virtual void SetAnnotation(const String& annotation)
      {
        if (!annotation.empty())
        {
          Context->OnString(IFF::Identifier::ANNOTATION, annotation);
        }
      }

      virtual void SetProperty(const String& property)
      {
        if (!property.empty())
        {
          Context->OnString(IFF::Identifier::PROPERTY, property);
        }
      }

      virtual void StartTrack(uint_t /*idx*/)
      {
        FinishTrack();
        Track = boost::make_shared<IFF::DataBuilder>(IFF::Identifier::TRACK);
        Context = Track;
      }
      
      virtual void SetData(Binary::Container::Ptr data)
      {
        Require(Context == Track);
        Track->OnChunk(IFF::Identifier::DATA, data);
      }
        
      virtual Binary::Data::Ptr GetResult()
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
      return boost::make_shared<BlobBuilder>();
    }
    
    class MetadataParser : public IFF::Visitor
    {
    public:
      explicit MetadataParser(Builder& delegate)
        : Delegate(delegate)
      {
      }
      
      virtual void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content)
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
          Delegate.SetProperty(IFF::GetString(*content));
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
      {
      }
      
      virtual void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content)
      {
        if (id == IFF::Identifier::DATA)
        {
          Require(!!content);
          Delegate.SetData(content);
        }
        else
        {
          Metadata.OnChunk(id, content);
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
        , CurTrack()
      {
      }
      
      virtual void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content)
      {
        if (id == IFF::Identifier::TRACK)
        {
          Require(!!content);
          ParseTrack(*content);
        }
        else
        {
          Metadata.OnChunk(id, content);
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
      uint_t CurTrack;
    };
    
    class FileParser : public IFF::Visitor
    {
    public:
      explicit FileParser(Builder& delegate)
        : Delegate(delegate)
        , CurTune()
      {
      }

      virtual void OnChunk(const IFF::Identifier::Type& id, Binary::Container::Ptr content)
      {
        Require(id == IFF::Identifier::MTC1);
        Require(!!content);
        ParseTune(*content);
      }
    private:
      void ParseTune(const Binary::Container& content)
      {
        Require(CurTune == 0);//only one tune is allowed
        ++CurTune;
        TuneParser tune(Delegate);
        IFF::Parse(content, tune);
      }
    private:
      Builder& Delegate;
      uint_t CurTune;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        FileParser file(target);
        const Binary::Container::Ptr result = IFF::Parse(data, file); 
        return CreateCalculatingCrcContainer(result, 0, result->Size());
      }
      catch (const std::exception& /*e*/)
      {
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace MultiTrackContainer

  Decoder::Ptr CreateMultiTrackContainerDecoder()
  {
    return boost::make_shared<MultiTrackContainer::Decoder>();
  }
}
}
