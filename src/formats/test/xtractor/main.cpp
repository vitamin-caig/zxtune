//common includes
#include <format.h>
#include <logging.h>
#include <parameters.h>
//library includes
#include <analysis/path.h>
#include <analysis/result.h>
#include <async/data_receiver.h>
#include <formats/archived_decoders.h>
#include <formats/packed_decoders.h>
#include <io/fs_tools.h>
#include <io/provider.h>
//std includes
#include <iostream>
#include <map>
//boost includes
#include <boost/make_shared.hpp>

//fix for new boost versions
namespace boost
{
  void tss_cleanup_implemented() { }
}

namespace
{
	void ShowError(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
	{
		std::cout << Error::AttributesToString(loc, code, text);
	}

	const std::string THIS_MODULE("XTractor");
}

template<class ObjType>
class ObjectsStorage
{
public:
  typedef boost::shared_ptr<const ObjectsStorage<ObjType> > Ptr;
  virtual ~ObjectsStorage() {}

  class Visitor
  {
  public:
    virtual ~Visitor() {}

    //return true if stop on current
    virtual bool Accept(ObjType object) = 0;
  };

  virtual void ForEach(Visitor& visitor) const = 0;
};

namespace Analysis
{
  class Point
  {
  public:
    typedef boost::shared_ptr<const Point> Ptr;
    virtual ~Point() {}

    virtual Path::Ptr GetPath() const = 0;
    virtual Binary::Container::Ptr GetData() const = 0;
  };

  Point::Ptr CreatePoint(Path::Ptr path, Binary::Container::Ptr data);
}

namespace
{
  class StaticPoint : public Analysis::Point
  {
  public:
    StaticPoint(Analysis::Path::Ptr path, Binary::Container::Ptr data)
      : Path(path)
      , Data(data)
    {
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      return Path;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }
  private:
    const Analysis::Path::Ptr Path;
    const Binary::Container::Ptr Data;
  };
}

namespace Analysis
{
  Point::Ptr CreatePoint(Path::Ptr path, Binary::Container::Ptr data)
  {
    return boost::make_shared<StaticPoint>(path, data);
  }
}

namespace Analysis
{
  class Service
  {
  public:
    typedef boost::shared_ptr<const Service> Ptr;
    virtual ~Service() {}

    virtual Result::Ptr Analyse(Point::Ptr point) const = 0;
  };
}

namespace Formats
{
  namespace Packed
  {
    typedef ObjectsStorage<Decoder::Ptr> DecodersStorage;

    DecodersStorage::Ptr GetAvailableDecoders();
  }
}

namespace Formats
{
  namespace Archived
  {
    typedef ObjectsStorage<Decoder::Ptr> DecodersStorage;

    DecodersStorage::Ptr GetAvailableDecoders();
  }
}

namespace
{
  class PackedDecodersStorage : public Formats::Packed::DecodersStorage
  {
  public:
    virtual void ForEach(Formats::Packed::DecodersStorage::Visitor& visitor) const
    {
      using namespace Formats::Packed;
      visitor.Accept(CreateCodeCruncher3Decoder()) ||
      visitor.Accept(CreateCompressorCode4Decoder()) ||
      visitor.Accept(CreateCompressorCode4PlusDecoder()) ||
      visitor.Accept(CreateDataSquieezerDecoder()) ||
      visitor.Accept(CreateESVCruncherDecoder()) ||
      visitor.Accept(CreateHrumDecoder()) ||
      visitor.Accept(CreateHrust1Decoder()) ||
      visitor.Accept(CreateHrust21Decoder()) ||
      visitor.Accept(CreateHrust23Decoder()) ||
      visitor.Accept(CreateLZSDecoder()) ||
      visitor.Accept(CreateMSPackDecoder()) ||
      visitor.Accept(CreatePowerfullCodeDecreaser61Decoder()) ||
      visitor.Accept(CreatePowerfullCodeDecreaser62Decoder()) ||
      visitor.Accept(CreateTRUSHDecoder()) ||
      visitor.Accept(CreateGamePackerDecoder()) ||
      visitor.Accept(CreateGamePackerPlusDecoder()) ||
      visitor.Accept(CreateTurboLZDecoder()) ||
      visitor.Accept(CreateTurboLZProtectedDecoder()) ||
      visitor.Accept(CreateCharPresDecoder()) ||
      visitor.Accept(CreatePack2Decoder()) ||
      visitor.Accept(CreateLZH1Decoder()) ||
      visitor.Accept(CreateLZH2Decoder()) ||
      visitor.Accept(CreateFullDiskImageDecoder()) ||
      visitor.Accept(CreateHobetaDecoder()) ||
      visitor.Accept(CreateSna128Decoder()) ||
      visitor.Accept(CreateTeleDiskImageDecoder())
      ;
    }
  };
}

namespace Formats
{
  namespace Packed
  {
    DecodersStorage::Ptr GetAvailableDecoders()
    {
      return boost::make_shared<PackedDecodersStorage>();
    }
  }
}

namespace
{
  class ArchivedDecodersStorage : public Formats::Archived::DecodersStorage
  {
  public:
    virtual void ForEach(Formats::Archived::DecodersStorage::Visitor& visitor) const
    {
      using namespace Formats::Archived;
      visitor.Accept(CreateZipDecoder()) ||
      visitor.Accept(CreateRarDecoder()) ||
      visitor.Accept(CreateZXZipDecoder()) ||
      visitor.Accept(CreateSCLDecoder()) ||
      visitor.Accept(CreateTRDDecoder()) ||
      visitor.Accept(CreateHripDecoder())
      //visitor.Accept(CreateAYDecoder()) ||
      ;
    }
  };
}

namespace Formats
{
  namespace Archived
  {
    DecodersStorage::Ptr GetAvailableDecoders()
    {
      return boost::make_shared<ArchivedDecodersStorage>();
    }
  }
}

namespace Parsing
{
  typedef DataReceiver<Analysis::Point::Ptr> Target;
  typedef DataTransmitter<Analysis::Point::Ptr> Source;
  typedef DataTransceiver<Analysis::Point::Ptr> Pipe;
}

namespace
{
  class SaveTarget : public Parsing::Target
  {
  public:
    explicit SaveTarget(const String& dir)
      : Dir(dir)
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Analysis::Path::Ptr path = point->GetPath();
      const Binary::Container::Ptr data = point->GetData();
      std::cout << path->AsString() << " " << data->Size() << " bytes" << std::endl;
      const String filePath = CreateFilePath(*path);
      const std::auto_ptr<std::ofstream> target = ZXTune::IO::CreateFile(filePath, true);
      target->write(static_cast<const char*>(data->Data()), data->Size());
    }

    virtual void Flush()
    {
    }
  private:
    String CreateFilePath(const Analysis::Path& path) const
    {
      const String asString = path.AsString();
      return ZXTune::IO::AppendPath(Dir, ZXTune::IO::MakePathFromString(asString, '_'));
    }
  private:
    const String Dir;
  };
}

namespace Parsing
{
  Parsing::Target::Ptr CreateSaveTarget(const String& dir)
  {
    return boost::make_shared<SaveTarget>(dir);
  }
}

namespace
{
  typedef boost::shared_ptr<const std::size_t> OffsetPtr;
  typedef boost::shared_ptr<std::size_t> OffsetRWPtr;

  class ScanningPoint : public Analysis::Point
  {
  public:
    ScanningPoint(Analysis::Path::Ptr path, Binary::Container::Ptr data, OffsetPtr offset)
      : Path(path)
      , Data(data)
      , Offset(offset)
    {
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      if (*Offset)
      {
        const String subpath = Strings::Format("+%1%", *Offset);
        return Path->Append(subpath);
      }
      return Path;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      if (const std::size_t offset = *Offset)
      {
        const std::size_t available = Data->Size();
        if (available > offset)
        {
          return Data->GetSubcontainer(offset, available - offset);
        }
        return Binary::Container::Ptr();
      }
      return Data;
    }
  private:
    const Analysis::Path::Ptr Path;
    const Binary::Container::Ptr Data;
    const OffsetPtr Offset;
  };

  typedef ObjectsStorage<Analysis::Service::Ptr> AnalyseServicesStorage;

  class AnalysedDataDispatcher : public AnalyseServicesStorage
                               , public Parsing::Source
  {
  public:
    typedef boost::shared_ptr<AnalysedDataDispatcher> Ptr;
  };

  class PackedDataAnalyseService : public Analysis::Service
  {
  public:
    PackedDataAnalyseService(Formats::Packed::Decoder::Ptr decoder, Parsing::Target::Ptr target)
      : Decoder(decoder)
      , Target(target)
    {
    }

    virtual Analysis::Result::Ptr Analyse(Analysis::Point::Ptr point) const
    {
      const Binary::Container::Ptr rawData = point->GetData();
      const String descr = Decoder->GetDescription();
      Log::Debug(THIS_MODULE, "Trying '%1%'", descr);
      if (Formats::Packed::Container::Ptr depacked = Decoder->Decode(*rawData))
      {
        const std::size_t matched = depacked->PackedSize();
        Log::Debug(THIS_MODULE, "Found in %1% bytes", matched);
        const Analysis::Path::Ptr curPath = point->GetPath();
        //TODO: parametrize
        const Analysis::Path::Ptr subPath = true ? curPath->Append(descr) : curPath;
        const Analysis::Point::Ptr resolved = Analysis::CreatePoint(subPath, depacked);
        Target->ApplyData(resolved);
        return Analysis::CreateMatchedResult(matched);
      }
      else
      {
        Log::Debug(THIS_MODULE, "Not found");
        return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
      }
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Parsing::Target::Ptr Target;
  };

  class PackedDataAdapter : public Formats::Packed::DecodersStorage::Visitor
  {
  public:
    PackedDataAdapter(AnalyseServicesStorage::Visitor& delegate, Parsing::Target::Ptr target)
      : Delegate(delegate)
      , Target(target)
    {
    }

    virtual bool Accept(Formats::Packed::Decoder::Ptr decoder)
    {
      const Analysis::Service::Ptr service = boost::make_shared<PackedDataAnalyseService>(decoder, Target);
      return Delegate.Accept(service);
    }
  private:
    AnalyseServicesStorage::Visitor& Delegate;
    const Parsing::Target::Ptr Target;
  };

  class PackedDataAnalysersStorage : public AnalysedDataDispatcher
  {
  public:
    PackedDataAnalysersStorage()
      : Target(Parsing::Target::CreateStub())
    {
    }

    virtual void ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      const Formats::Packed::DecodersStorage::Ptr decoders = Formats::Packed::GetAvailableDecoders();
      PackedDataAdapter adapter(visitor, Target);
      decoders->ForEach(adapter);
    }
    
    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Parsing::Target::Ptr Target;
  };

  class ArchivedDataAnalyseService : public Analysis::Service
  {
  public:
    ArchivedDataAnalyseService(Formats::Archived::Decoder::Ptr decoder, Parsing::Target::Ptr target)
      : Decoder(decoder)
      , Target(target)
    {
    }

    virtual Analysis::Result::Ptr Analyse(Analysis::Point::Ptr point) const
    {
      const Binary::Container::Ptr rawData = point->GetData();
      const String descr = Decoder->GetDescription();
      Log::Debug(THIS_MODULE, "Trying '%1%'", descr);
      if (Formats::Archived::Container::Ptr depacked = Decoder->Decode(*rawData))
      {
        const std::size_t matched = depacked->Size();
        Log::Debug(THIS_MODULE, "Found in %1% bytes", matched);
        const Analysis::Path::Ptr curPath = point->GetPath();
        //TODO: parametrize
        const Analysis::Path::Ptr subPath = true ? curPath->Append(descr) : curPath;
        const DepackFiles walker(subPath, Target);
        depacked->ExploreFiles(walker);
        return Analysis::CreateMatchedResult(matched);
      }
      else
      {
        Log::Debug(THIS_MODULE, "Not found");
        return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
      }
    }
  private:
    class DepackFiles : public Formats::Archived::Container::Walker
    {
    public:
      DepackFiles(Analysis::Path::Ptr path, Parsing::Target::Ptr target)
        : Path(path)
        , Target(target)
      {
      }

      virtual void OnFile(const Formats::Archived::File& file) const
      {
        if (Binary::Container::Ptr data = file.GetData())
        {
          const Analysis::Path::Ptr subpath = Path->Append(file.GetName());
          const Analysis::Point::Ptr point = Analysis::CreatePoint(subpath, data);
          Target->ApplyData(point);
        }
      }
    private:
      const Analysis::Path::Ptr Path;
      const Parsing::Target::Ptr Target;
    };
  private:
    const Formats::Archived::Decoder::Ptr Decoder;
    const Parsing::Target::Ptr Target;
  };

  class ArchivedDataAdapter : public Formats::Archived::DecodersStorage::Visitor
  {
  public:
    ArchivedDataAdapter(AnalyseServicesStorage::Visitor& delegate, Parsing::Target::Ptr target)
      : Delegate(delegate)
      , Target(target)
    {
    }

    virtual bool Accept(Formats::Archived::Decoder::Ptr decoder)
    {
      const Analysis::Service::Ptr service = boost::make_shared<ArchivedDataAnalyseService>(decoder, Target);
      return Delegate.Accept(service);
    }
  private:
    AnalyseServicesStorage::Visitor& Delegate;
    const Parsing::Target::Ptr Target;
  };

  class ArchivedDataAnalysersStorage : public AnalysedDataDispatcher
  {
  public:
    ArchivedDataAnalysersStorage()
      : Target(Parsing::Target::CreateStub())
    {
    }

    virtual void ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      const Formats::Archived::DecodersStorage::Ptr decoders = Formats::Archived::GetAvailableDecoders();
      ArchivedDataAdapter adapter(visitor, Target);
      decoders->ForEach(adapter);
    }

    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Parsing::Target::Ptr Target;
  };

  class ScanningDataAnalyser : private AnalyseServicesStorage::Visitor
  {
  public:
    ScanningDataAnalyser(Analysis::Point::Ptr source, AnalyseServicesStorage::Ptr services, Parsing::Target::Ptr unresolved)
      : Services(services)
      , ScanOffset(boost::make_shared<std::size_t>(0))
      , ScanSource(boost::make_shared<ScanningPoint>(source->GetPath(), source->GetData(), ScanOffset))
      , Unresolved(unresolved)
      , Limit(source->GetData()->Size())
      , LastUnresolvedOffset(boost::make_shared<std::size_t>())
      , LastUnresolvedSource(boost::make_shared<ScanningPoint>(source->GetPath(), source->GetData(), LastUnresolvedOffset))
    {
    }

    void Analyse()
    {
      *ScanOffset = *LastUnresolvedOffset = 0;
      Services->ForEach(*this);
      while (Binary::Container::Ptr data = ScanSource->GetData())
      {
        const std::size_t minOffset = Analysers.empty() ? Limit : Analysers.begin()->first;
        if (minOffset > *ScanOffset)
        {
          *ScanOffset = minOffset;
          continue;
        }
        for (AnalysersCache::iterator it = Analysers.begin(); it != Analysers.end(); )
        {
          const AnalysersCache::iterator toProcess = it;
          ++it;
          if (toProcess->first <= *ScanOffset)
          {
            const Analysis::Service::Ptr service = toProcess->second;
            Analysers.erase(toProcess);
            if (Accept(service))
            {
              break;
            }
          }
        }
      }
      ProcessUnresolvedTill(Limit);
    }
  private:
    virtual bool Accept(Analysis::Service::Ptr service)
    {
      const Analysis::Result::Ptr result = service->Analyse(ScanSource);
      if (const std::size_t matched = result->GetMatchedDataSize())
      {
        const std::size_t nextOffset = *ScanOffset + matched;
        ProcessUnresolvedTill(*ScanOffset);
        *ScanOffset = *LastUnresolvedOffset = nextOffset;
        if (nextOffset == Limit)
        {
          Log::Debug(THIS_MODULE, "No more data to analyze. Stop.");
          return true;
        }
        Analysers.insert(std::make_pair(*ScanOffset, service));
      }
      else
      {
        const std::size_t lookahead = result->GetLookaheadOffset();
        const std::size_t nextSearch = *ScanOffset + std::max(lookahead, std::size_t(1));
        if (nextSearch < Limit)
        {
          Log::Debug(THIS_MODULE, "Skip for nearest %1% bytes", lookahead);
          Analysers.insert(std::make_pair(nextSearch, service));
        }
        else
        {
          Log::Debug(THIS_MODULE, "Disable for further checking");
        }
      }
      return false;
    }

    void ProcessUnresolvedTill(std::size_t offset) const
    {
      if (*LastUnresolvedOffset < offset)
      {
        const std::size_t unresolvedSize = offset - *LastUnresolvedOffset;
        const Analysis::Point::Ptr unresolved = Analysis::CreatePoint(LastUnresolvedSource->GetPath(), LastUnresolvedSource->GetData()->GetSubcontainer(0, unresolvedSize));
        Unresolved->ApplyData(unresolved);
        *LastUnresolvedOffset = offset;
      }
    }
  private:
    const AnalyseServicesStorage::Ptr Services;
    const OffsetRWPtr ScanOffset;
    const Analysis::Point::Ptr ScanSource;
    const Parsing::Target::Ptr Unresolved;
    const std::size_t Limit;
    const OffsetRWPtr LastUnresolvedOffset;
    const Analysis::Point::Ptr LastUnresolvedSource;
    typedef std::multimap<std::size_t, Analysis::Service::Ptr> AnalysersCache;
    AnalysersCache Analysers;
  };

  class UnresolvedData : public Parsing::Pipe
  {
  public:
    explicit UnresolvedData(AnalyseServicesStorage::Ptr services)
      : Services(services)
      , Target(Parsing::Target::CreateStub())
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      ScanningDataAnalyser analyser(point, Services, Target);
      analyser.Analyse();
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    const AnalyseServicesStorage::Ptr Services;
    Parsing::Target::Ptr Target;
  };

  typedef DataReceiver<String> StringsReceiver;

  class OpenPoint : public StringsReceiver
                  , public Parsing::Source
  {
  public:
    typedef boost::shared_ptr<OpenPoint> Ptr;
  };

  class OpenPointImpl : public OpenPoint
  {
  public:
    OpenPointImpl()
      : Target(Parsing::Target::CreateStub())
      , Params(Parameters::Container::Create())
    {
    }

    virtual void ApplyData(const String& path)
    {
      Log::Debug(THIS_MODULE, "Opening '%1%'", path);
      Binary::Container::Ptr data;
      ThrowIfError(ZXTune::IO::OpenData(path, *Params, ZXTune::IO::ProgressCallback(), data));
      String dir;
      const String& filename = ZXTune::IO::ExtractLastPathComponent(path, dir);
      const Analysis::Point::Ptr point = Analysis::CreatePoint(Analysis::ParsePath(filename), data);
      Target->ApplyData(point);
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Parsing::Target::Ptr Target;
    const Parameters::Accessor::Ptr Params;
  };

  class Valve : public Parsing::Pipe
  {
  public:
    Valve()
      : Target(Parsing::Target::CreateStub())
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& data)
    {
      Target->ApplyData(data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Parsing::Target::Ptr Target;
  };

  Parsing::Target::Ptr AsyncWrap(std::size_t threads, Parsing::Target::Ptr target)
  {
    return Async::DataReceiver<Analysis::Point::Ptr>::Create(threads, target);
  }
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return 0;
  }
  try
  {
    const std::size_t saveThreads = 0;
    /*

    filename -> rawData -> unarchive -(unresolved)-> depack -(unresolved)-> save
                |<---------+                         |
                +<-----------------------------------+

    */
    const OpenPoint::Ptr open = boost::make_shared<OpenPointImpl>();
    const Parsing::Target::Ptr save = Parsing::CreateSaveTarget("1");
    const Parsing::Target::Ptr asyncSave = AsyncWrap(saveThreads, save);

    const AnalysedDataDispatcher::Ptr unarchive = boost::make_shared<ArchivedDataAnalysersStorage>();
    const Parsing::Pipe::Ptr filterArchives = boost::make_shared<UnresolvedData>(unarchive);
    const AnalysedDataDispatcher::Ptr depack = boost::make_shared<PackedDataAnalysersStorage>();
    const Parsing::Pipe::Ptr filterPacked = boost::make_shared<UnresolvedData>(depack);

    const Parsing::Pipe::Ptr rawData = boost::make_shared<Valve>();
    rawData->SetTarget(filterArchives);
    //unarchive -(unresolved)-> depack
    filterArchives->SetTarget(filterPacked);
    //depack -(unresolved)-> save
    filterPacked->SetTarget(asyncSave);
    //unarchive -> rawData
    unarchive->SetTarget(rawData);
    //depack -> rawData
    depack->SetTarget(rawData);

    //filename -> data
    open->SetTarget(rawData);

    for (int idx = 1; idx < argc; ++idx)
    {
      const String file(argv[idx]);
      open->ApplyData(file);
    }
    rawData->Flush();
    asyncSave->Flush();
    //break cycle
    rawData->SetTarget(Parsing::Target::CreateStub());
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(&ShowError);
  }
}