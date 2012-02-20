/*
Abstract:
  XTractor tool main file

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <apps/version/api.h>
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
#include <set>
//boost includes
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
//text includes
#include "text/text.h"

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

  virtual bool ForEach(Visitor& visitor) const = 0;
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
    virtual bool ForEach(Formats::Packed::DecodersStorage::Visitor& visitor) const
    {
      using namespace Formats::Packed;
      return visitor.Accept(CreateCodeCruncher3Decoder())
          || visitor.Accept(CreateCompressorCode4Decoder())
          || visitor.Accept(CreateCompressorCode4PlusDecoder())
          || visitor.Accept(CreateDataSquieezerDecoder())
          || visitor.Accept(CreateESVCruncherDecoder())
          || visitor.Accept(CreateHrumDecoder())
          || visitor.Accept(CreateHrust1Decoder())
          || visitor.Accept(CreateHrust21Decoder())
          || visitor.Accept(CreateHrust23Decoder())
          || visitor.Accept(CreateLZSDecoder())
          || visitor.Accept(CreateMSPackDecoder())
          || visitor.Accept(CreatePowerfullCodeDecreaser61Decoder())
          || visitor.Accept(CreatePowerfullCodeDecreaser62Decoder())
          || visitor.Accept(CreateTRUSHDecoder())
          || visitor.Accept(CreateGamePackerDecoder())
          || visitor.Accept(CreateGamePackerPlusDecoder())
          || visitor.Accept(CreateTurboLZDecoder())
          || visitor.Accept(CreateTurboLZProtectedDecoder())
          || visitor.Accept(CreateCharPresDecoder())
          || visitor.Accept(CreatePack2Decoder())
          || visitor.Accept(CreateLZH1Decoder())
          || visitor.Accept(CreateLZH2Decoder())
          || visitor.Accept(CreateFullDiskImageDecoder())
          || visitor.Accept(CreateHobetaDecoder())
          || visitor.Accept(CreateSna128Decoder())
          || visitor.Accept(CreateTeleDiskImageDecoder())
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
    virtual bool ForEach(Formats::Archived::DecodersStorage::Visitor& visitor) const
    {
      using namespace Formats::Archived;
      return visitor.Accept(CreateZipDecoder())
          || visitor.Accept(CreateRarDecoder())
          || visitor.Accept(CreateZXZipDecoder())
          || visitor.Accept(CreateSCLDecoder())
          || visitor.Accept(CreateTRDDecoder())
          || visitor.Accept(CreateHripDecoder())
      //  || visitor.Accept(CreateAYDecoder())
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
    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Analysis::Path::Ptr path = point->GetPath();
      const String filePath = path->AsString();
      const Binary::Container::Ptr data = point->GetData();
      const std::auto_ptr<std::ofstream> target = ZXTune::IO::CreateFile(filePath, true);
      target->write(static_cast<const char*>(data->Data()), data->Size());
    }

    virtual void Flush()
    {
    }
  };

  class SizeFilterTarget : public Parsing::Target
  {
  public:
    SizeFilterTarget(std::size_t minSize, Parsing::Target::Ptr target)
      : MinSize(minSize)
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Binary::Container::Ptr data = point->GetData();
      if (data->Size() >= MinSize)
      {
        Target->ApplyData(point);
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const std::size_t MinSize;
    const Parsing::Target::Ptr Target;
  };

  class SkipEmptyDataTarget : public Parsing::Target
  {
  public:
    explicit SkipEmptyDataTarget(Parsing::Target::Ptr target)
      : Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Binary::Container::Ptr data = point->GetData();
      const uint8_t* const begin = static_cast<const uint8_t*>(data->Data());
      const uint8_t* const end = begin + data->Size();
      if (end != std::find_if(begin, end, std::bind1st(std::not_equal_to<uint8_t>(), *begin)))
      {
        Target->ApplyData(point);
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Parsing::Target::Ptr Target;
  };

  class BuildDirsTarget : public Parsing::Target
  {
  public:
    BuildDirsTarget(const String& dir, Parsing::Target::Ptr target)
      : Dir(dir)
      , Target(target)
    {
      CreateDir(Dir);
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Analysis::Path::Ptr oldPath = point->GetPath();
      Analysis::Path::Ptr newPath = Analysis::ParsePath(Dir);
      for (Analysis::Path::Iterator::Ptr iter = oldPath->GetIterator(); iter->IsValid();)
      {
        const String component = ZXTune::IO::MakePathFromString(iter->Get(), '_');
        iter->Next();
        newPath = newPath->Append(component);
        if (iter->IsValid())
        {
          CreateDir(newPath->AsString());
        }
      }
      const Analysis::Point::Ptr result = Analysis::CreatePoint(newPath, point->GetData());
      Target->ApplyData(result);
    }

    virtual void Flush()
    {
      Target->Flush();
    }

  private:
    void CreateDir(const String& dir)
    {
      const boost::mutex::scoped_lock lock(DirCacheLock);
      if (DirCache.count(dir))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "Creating dir '%1%'", dir);
      const boost::filesystem::path path(dir);
      if (boost::filesystem::create_directories(path))
      {
        DirCache.insert(dir);
      }
    }
  private:
    const String Dir;
    const Parsing::Target::Ptr Target;
    boost::mutex DirCacheLock;
    std::set<String> DirCache;
  };
}

namespace Parsing
{
  Parsing::Target::Ptr CreateSaveTarget()
  {
    return boost::make_shared<SaveTarget>();
  }

  Parsing::Target::Ptr CreateSizeFilterTarget(std::size_t minSize, Parsing::Target::Ptr target)
  {
    return boost::make_shared<SizeFilterTarget>(minSize, target);
  }

  Parsing::Target::Ptr CreateBuildDirsTarget(const String& dir, Parsing::Target::Ptr target)
  {
    return boost::make_shared<BuildDirsTarget>(dir, target);
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
      const String subpath = Strings::Format("+%1%", *Offset);
      return Path->Append(subpath);
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

    virtual bool ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      const Formats::Packed::DecodersStorage::Ptr decoders = Formats::Packed::GetAvailableDecoders();
      PackedDataAdapter adapter(visitor, Target);
      return decoders->ForEach(adapter);
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

    virtual bool ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      const Formats::Archived::DecodersStorage::Ptr decoders = Formats::Archived::GetAvailableDecoders();
      ArchivedDataAdapter adapter(visitor, Target);
      return decoders->ForEach(adapter);
    }

    virtual void SetTarget(Parsing::Target::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Parsing::Target::Ptr Target;
  };

  class CompositeAnalyseServicesStorage : public AnalyseServicesStorage
  {
  public:
    CompositeAnalyseServicesStorage(AnalyseServicesStorage::Ptr first, AnalyseServicesStorage::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual bool ForEach(AnalyseServicesStorage::Visitor &visitor) const
    {
      return First->ForEach(visitor) || Second->ForEach(visitor);
    }
  private:
    const AnalyseServicesStorage::Ptr First;
    const AnalyseServicesStorage::Ptr Second;
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

  typedef DataTransceiver<String, Analysis::Point::Ptr> OpenPoint;

  class PathFactory
  {
  public:
    typedef boost::shared_ptr<const PathFactory> Ptr;
    virtual ~PathFactory() {}

    virtual Analysis::Path::Ptr CreatePath(const String& filename) const = 0;
  };

  class FilenamePathFactory : public PathFactory
  {
  public:
    virtual Analysis::Path::Ptr CreatePath(const String& filename) const
    {
      String dir;
      const String name = ZXTune::IO::ExtractLastPathComponent(filename, dir);
      return Analysis::ParsePath(name);
    }
  };

  class FlatPathFactory : public PathFactory
  {
  public:
    virtual Analysis::Path::Ptr CreatePath(const String& filename) const
    {
      const String fullPath = ZXTune::IO::MakePathFromString(filename, '_');
      return Analysis::ParsePath(fullPath);
    }
  };

  class FullPathFactory : public PathFactory
  {
  public:
    virtual Analysis::Path::Ptr CreatePath(const String& filename) const
    {
      return Analysis::ParsePath(filename);
    }
  };

  class OpenPointImpl : public OpenPoint
  {
  public:
    explicit OpenPointImpl(PathFactory::Ptr factory)
      : Factory(factory)
      , Target(Parsing::Target::CreateStub())
      , Params(Parameters::Container::Create())
    {
    }

    virtual void ApplyData(const String& filename)
    {
      Log::Debug(THIS_MODULE, "Opening '%1%'", filename);
      Binary::Container::Ptr data;
      ThrowIfError(ZXTune::IO::OpenData(filename, *Params, ZXTune::IO::ProgressCallback(), data));
      const Analysis::Path::Ptr path = Factory->CreatePath(filename);
      const Analysis::Point::Ptr point = Analysis::CreatePoint(path, data);
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
    const PathFactory::Ptr Factory;
    Parsing::Target::Ptr Target;
    const Parameters::Accessor::Ptr Params;
  };

  class TargetNamePoint : public Parsing::Target
  {
  public:
    TargetNamePoint(PathFactory::Ptr factory, Parsing::Target::Ptr target)
      : Factory(factory)
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Point::Ptr& point)
    {
      const Analysis::Path::Ptr oldPath = point->GetPath();
      const String oldPathString = oldPath->AsString();
      const Analysis::Path::Ptr newPath = Factory->CreatePath(oldPathString);
      const Analysis::Point::Ptr result = Analysis::CreatePoint(newPath, point->GetData());
      Target->ApplyData(result);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const PathFactory::Ptr Factory;
    const Parsing::Target::Ptr Target;
  };

  class Valve : public Parsing::Pipe
  {
  public:
    explicit Valve(Parsing::Target::Ptr target = Parsing::Target::CreateStub())
      : Target(target)
    {
    }

    virtual ~Valve()
    {
      //break possible cycles
      Target = Parsing::Target::CreateStub();
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

  class ResolveDirsPoint : public DataTransceiver<String>
  {
  public:
    ResolveDirsPoint()
      : Target(StringsReceiver::CreateStub())
    {
    }

    virtual void ApplyData(const String& filename)
    {
      const boost::filesystem::path path(filename);
      for (boost::filesystem::recursive_directory_iterator iter(path, boost::filesystem::symlink_option::recurse), lim = boost::filesystem::recursive_directory_iterator();
           iter != lim; ++iter)
      {
        const boost::filesystem::path subpath = iter->path();
        const String subPathString = subpath.string<String>();
        const boost::filesystem::file_type type = iter->status().type();
        if (type == boost::filesystem::regular_file)
        {
          Target->ApplyData(subPathString);
        }
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(StringsReceiver::Ptr target)
    {
      Target = target;
    }

  private:
    StringsReceiver::Ptr Target;
  };

  template<class Object>
  typename DataReceiver<Object>::Ptr AsyncWrap(std::size_t threads, std::size_t queueSize, typename DataReceiver<Object>::Ptr target)
  {
    return Async::DataReceiver<Object>::Create(threads, queueSize, target);
  }

  class TargetOptions
  {
  public:
    virtual ~TargetOptions() {}

    virtual String TargetDir() const = 0;
    virtual bool FullSourcePath() const = 0;
    virtual bool FlatSubpath() const = 0;
    virtual bool IgnoreEmptyData() const = 0;
    virtual std::size_t MinDataSize() const = 0;
    virtual std::size_t SaveThreadsCount() const = 0;
    virtual std::size_t SaveDataQueueSize() const = 0;
  };

  class AnalysisOptions
  {
  public:
    virtual ~AnalysisOptions() {}

    virtual std::size_t AnalysisThreads() const = 0;
    virtual std::size_t AnalysisDataQueueSize() const = 0;
  };

  Parsing::Target::Ptr CreateTarget(const TargetOptions& opts)
  {
    const PathFactory::Ptr dstFactory = opts.FlatSubpath()
      ? PathFactory::Ptr(boost::make_shared<FlatPathFactory>())
      : PathFactory::Ptr(boost::make_shared<FullPathFactory>());
    const Parsing::Target::Ptr save = Parsing::CreateSaveTarget();
    const Parsing::Target::Ptr makeDirs = Parsing::CreateBuildDirsTarget(opts.TargetDir(), save);
    const Parsing::Target::Ptr makeName = boost::make_shared<TargetNamePoint>(dstFactory, makeDirs);
    const Parsing::Target::Ptr storeAll = makeName;
    const Parsing::Target::Ptr storeNoEmpty = opts.IgnoreEmptyData()
      ? Parsing::Target::Ptr(boost::make_shared<SkipEmptyDataTarget>(storeAll))
      : storeAll;
    const std::size_t minSize = opts.MinDataSize();
    const Parsing::Target::Ptr storeEnoughSize = minSize
      ? Parsing::Target::Ptr(boost::make_shared<SizeFilterTarget>(minSize, storeNoEmpty))
      : storeNoEmpty;
    const Parsing::Target::Ptr result = storeEnoughSize;
    return AsyncWrap<Analysis::Point::Ptr>(opts.SaveThreadsCount(), opts.SaveDataQueueSize(), result);;
  }

  template<class InType, class OutType = InType>
  class TransceivePipe : public DataTransceiver<InType, OutType>
  {
  public:
    TransceivePipe(typename DataReceiver<InType>::Ptr input, typename DataTransmitter<OutType>::Ptr output)
      : Input(input)
      , Output(output)
    {
    }

    virtual void ApplyData(const InType& data)
    {
      Input->ApplyData(data);
    }

    virtual void Flush()
    {
      Input->Flush();
    }

    virtual void SetTarget(typename DataReceiver<OutType>::Ptr target)
    {
      Output->SetTarget(target);
    }
  private:
    const typename DataReceiver<InType>::Ptr Input;
    const typename DataTransmitter<OutType>::Ptr Output;
  };

  Parsing::Pipe::Ptr CreateAnalyser(const AnalysisOptions& opts)
  {
    //form analysers
    const AnalysedDataDispatcher::Ptr unarchive = boost::make_shared<ArchivedDataAnalysersStorage>();
    const AnalysedDataDispatcher::Ptr depack = boost::make_shared<PackedDataAnalysersStorage>();
    const AnalyseServicesStorage::Ptr anyConversion = boost::make_shared<CompositeAnalyseServicesStorage>(unarchive, depack);
    const Parsing::Pipe::Ptr filterUnresolved = boost::make_shared<UnresolvedData>(anyConversion);

    const Parsing::Pipe::Ptr unknownData = boost::make_shared<Valve>();
    unknownData->SetTarget(filterUnresolved);
    unarchive->SetTarget(unknownData);
    depack->SetTarget(unknownData);
    const Parsing::Target::Ptr input = AsyncWrap<Analysis::Point::Ptr>(opts.AnalysisThreads(), opts.AnalysisDataQueueSize(), unknownData);
    return boost::make_shared<TransceivePipe<Analysis::Point::Ptr> >(input, filterUnresolved);
  }

  OpenPoint::Ptr CreateSource(const TargetOptions& opts)
  {
    const PathFactory::Ptr srcFactory = opts.FullSourcePath()
      ? PathFactory::Ptr(boost::make_shared<FullPathFactory>())
      : PathFactory::Ptr(boost::make_shared<FilenamePathFactory>());
    const OpenPoint::Ptr open = boost::make_shared<OpenPointImpl>(srcFactory);
    const boost::shared_ptr<ResolveDirsPoint> resolve = boost::make_shared<ResolveDirsPoint>();
    resolve->SetTarget(open);
    return boost::make_shared<TransceivePipe<String, Analysis::Point::Ptr> >(resolve, open);
  }

  class Options : public AnalysisOptions
                , public TargetOptions
  {
  public:
    Options()
      : AnalysisThreadsValue(1)
      , AnalysisDataQueueSizeValue(10)
      , TargetDirValue(Text::DEFAULT_RESULT_FOLDER)
      , FullSourcePathValue(false)
      , FlatSubpathValue(false)
      , IgnoreEmptyDataValue(false)
      , MinDataSizeValue(0)
      , SaveThreadsCountValue(1)
      , SaveDataQueueSizeValue(500)
      //cmdline
      , OptionsDescription(Text::TARGET_SECTION)
    {
      using namespace boost::program_options;
      OptionsDescription.add_options()
        (Text::ANALYSIS_THREADS_KEY, value<std::size_t>(&AnalysisThreadsValue), Text::ANALYSIS_THREADS_DESC)
        (Text::ANALYSIS_QUEUE_SIZE_KEY, value<std::size_t>(&AnalysisDataQueueSizeValue), Text::ANALYSIS_QUEUE_SIZE_DESC)
        (Text::TARGET_DIR_KEY, value<String>(&TargetDirValue), Text::TARGET_DIR_DESC)
        (Text::FULL_SOURCE_PATH_KEY, bool_switch(&FullSourcePathValue), Text::FULL_SOURCE_PATH_DESC)
        (Text::FLAT_SUBPATH_KEY, bool_switch(&FlatSubpathValue), Text::FLAT_SUBPATH_DESC)
        (Text::IGNORE_EMPTY_KEY, bool_switch(&IgnoreEmptyDataValue), Text::IGNORE_EMPTY_DESC)
        (Text::MINIMAL_SIZE_KEY, value<std::size_t>(&MinDataSizeValue), Text::MINIMAL_SIZE_DESC)
        (Text::SAVE_THREADS_KEY, value<std::size_t>(&SaveThreadsCountValue), Text::SAVE_THREADS_DESC)
        (Text::SAVE_QUEUE_SIZE_KEY, value<std::size_t>(&SaveDataQueueSizeValue), Text::SAVE_QUEUE_SIZE_DESC)
       ;
    }

    virtual std::size_t AnalysisThreads() const
    {
      return AnalysisThreadsValue;
    }

    virtual std::size_t AnalysisDataQueueSize() const
    {
      return AnalysisDataQueueSizeValue;
    }

    virtual String TargetDir() const
    {
      return TargetDirValue;
    }

    virtual bool FullSourcePath() const
    {
      return FullSourcePathValue;
    }

    virtual bool FlatSubpath() const
    {
      return FlatSubpathValue;
    }

    virtual bool IgnoreEmptyData() const
    {
      return IgnoreEmptyDataValue;
    }

    virtual std::size_t MinDataSize() const
    {
      return MinDataSizeValue;
    }

    virtual std::size_t SaveThreadsCount() const
    {
      return SaveThreadsCountValue;
    }

    virtual std::size_t SaveDataQueueSize() const
    {
      return SaveDataQueueSizeValue;
    }

    const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
  private:
    std::size_t AnalysisThreadsValue;
    std::size_t AnalysisDataQueueSizeValue;
    String TargetDirValue;
    bool FullSourcePathValue;
    bool FlatSubpathValue;
    bool IgnoreEmptyDataValue;
    std::size_t MinDataSizeValue;
    std::size_t SaveThreadsCountValue;
    std::size_t SaveDataQueueSizeValue;
    boost::program_options::options_description OptionsDescription;
  };
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return 0;
  }
  try
  {
    /*

                                           analyseThreads                 saveThreads
                                             |                              |
    file/dir -> [resolve] -> name -> [open] -*-> data -> [convert] -> data -*-> [filter] -> [createDir] -> [save]
                                        |           |            |                             |       |
                                       factory      +<-converted-+                            factory directory

    */

    const Options opts;
    StringArray paths;
    {
      using namespace boost::program_options;
      options_description options(Strings::Format(Text::USAGE_SECTION, *argv));
      options.add_options()
        (Text::HELP_KEY, Text::HELP_DESC)
        (Text::VERSION_KEY, Text::VERSION_DESC)
      ;
      options.add(opts.GetOptionsDescription());
      options.add_options()
        (Text::INPUT_KEY, value<StringArray>(&paths), Text::INPUT_DESC)
      ;
      positional_options_description inputPositional;
      inputPositional.add(Text::INPUT_KEY, -1);

      variables_map vars;
      store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
      notify(vars);
      if (vars.count(Text::HELP_KEY))
      {
        std::cout << options << std::endl;
        return true;
      }
      else if (vars.count(Text::VERSION_KEY))
      {
        std::cout << GetProgramVersionString() << std::endl;
        return true;
      }
    }

    const Parsing::Target::Ptr result = CreateTarget(opts);
    const Parsing::Pipe::Ptr analyse = CreateAnalyser(opts);
    const OpenPoint::Ptr input = CreateSource(opts);

    input->SetTarget(analyse);
    analyse->SetTarget(result);

    std::for_each(paths.begin(), paths.end(), boost::bind(&StringsReceiver::ApplyData, input.get(), _1));
    input->Flush();
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(&ShowError);
  }
}
