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
#include <debug_log.h>
#include <parameters.h>
#include <progress_callback.h>
//library includes
#include <analysis/path.h>
#include <analysis/result.h>
#include <async/data_receiver.h>
#include <formats/archived/decoders.h>
#include <formats/packed/decoders.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <strings/array.h>
#include <strings/fields.h>
#include <strings/format.h>
#include <strings/template.h>
//std includes
#include <iostream>
#include <locale>
#include <map>
#include <numeric>
#include <set>
//boost includes
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("XTractor");
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
  class Node
  {
  public:
    typedef boost::shared_ptr<const Node> Ptr;
    virtual ~Node() {}

    //! Name to distinguish. Can be empty
    virtual String Name() const = 0;
    //! Data associated with. Cannot be empty
    virtual Binary::Container::Ptr Data() const = 0;
    //! Provider identifier in any form. Can be empty
    virtual String Provider() const = 0;
    //! Parent node. Ptr() if root node
    virtual Ptr Parent() const = 0;
  };

  Node::Ptr CreateRootNode(Binary::Container::Ptr data, const String& name);
  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name, const String& provider);
  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name);
}

namespace
{
  class RootNode : public Analysis::Node
  {
  public:
    RootNode(Binary::Container::Ptr data, const String& name)
      : DataVal(data)
      , NameVal(name)
    {
    }

    virtual String Name() const
    {
      return NameVal;
    }

    virtual Binary::Container::Ptr Data() const
    {
      return DataVal;
    }

    virtual String Provider() const
    {
      return String();
    }

    virtual Analysis::Node::Ptr Parent() const
    {
      return Analysis::Node::Ptr();
    }
  private:
    const Binary::Container::Ptr DataVal;
    const String NameVal;
  };

  class SubNode : public Analysis::Node
  {
  public:
    SubNode(Analysis::Node::Ptr parent, Binary::Container::Ptr data, const String& name, const String& provider)
      : ParentVal(parent)
      , DataVal(data)
      , NameVal(name)
      , ProviderVal(provider)
    {
    }

    virtual String Name() const
    {
      return NameVal;
    }

    virtual Binary::Container::Ptr Data() const
    {
      return DataVal;
    }

    virtual String Provider() const
    {
      return ProviderVal;
    }

    virtual Analysis::Node::Ptr Parent() const
    {
      return ParentVal;
    }
  private:
    const Analysis::Node::Ptr ParentVal;
    const Binary::Container::Ptr DataVal;
    const String NameVal;
    const String ProviderVal;
  };
}

namespace Analysis
{
  //since data is required, place it first
  Node::Ptr CreateRootNode(Binary::Container::Ptr data, const String& name)
  {
    return boost::make_shared<RootNode>(data, name);
  }

  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name, const String& provider)
  {
    return boost::make_shared<SubNode>(parent, data, name, provider);
  }

  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name)
  {
    return CreateSubnode(parent, data, name, parent->Provider());
  }
}

namespace Analysis
{
  typedef DataReceiver<Node::Ptr> NodeReceiver;
  typedef DataTransmitter<Node::Ptr> NodeTransmitter;
  typedef DataTransceiver<Node::Ptr> NodeTransceiver;
}

namespace Analysis
{
  class Service
  {
  public:
    typedef boost::shared_ptr<const Service> Ptr;
    virtual ~Service() {}

    virtual Result::Ptr Analyse(Node::Ptr node) const = 0;
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
    PackedDecodersStorage()
    {
      using namespace Formats::Packed;
      Decoders.push_back(CreateCodeCruncher3Decoder());
      Decoders.push_back(CreateCompressorCode4Decoder());
      Decoders.push_back(CreateCompressorCode4PlusDecoder());
      Decoders.push_back(CreateDataSquieezerDecoder());
      Decoders.push_back(CreateESVCruncherDecoder());
      Decoders.push_back(CreateHrumDecoder());
      Decoders.push_back(CreateHrust1Decoder());
      Decoders.push_back(CreateHrust21Decoder());
      Decoders.push_back(CreateHrust23Decoder());
      Decoders.push_back(CreateLZSDecoder());
      Decoders.push_back(CreateMSPackDecoder());
      Decoders.push_back(CreatePowerfullCodeDecreaser61Decoder());
      Decoders.push_back(CreatePowerfullCodeDecreaser62Decoder());
      Decoders.push_back(CreateTRUSHDecoder());
      Decoders.push_back(CreateGamePackerDecoder());
      Decoders.push_back(CreateGamePackerPlusDecoder());
      Decoders.push_back(CreateTurboLZDecoder());
      Decoders.push_back(CreateTurboLZProtectedDecoder());
      Decoders.push_back(CreateCharPresDecoder());
      Decoders.push_back(CreatePack2Decoder());
      Decoders.push_back(CreateLZH1Decoder());
      Decoders.push_back(CreateLZH2Decoder());
      Decoders.push_back(CreateFullDiskImageDecoder());
      Decoders.push_back(CreateHobetaDecoder());
      Decoders.push_back(CreateSna128Decoder());
      Decoders.push_back(CreateTeleDiskImageDecoder());
      Decoders.push_back(CreateZ80V145Decoder());
      Decoders.push_back(CreateZ80V20Decoder());
      Decoders.push_back(CreateZ80V30Decoder());
      Decoders.push_back(CreateMegaLZDecoder());
    }

    virtual bool ForEach(Formats::Packed::DecodersStorage::Visitor& visitor) const
    {
      return Decoders.end() != std::find_if(Decoders.begin(), Decoders.end(),
        boost::bind(&Formats::Packed::DecodersStorage::Visitor::Accept, &visitor, _1));
    }
  private:
    std::vector<Formats::Packed::Decoder::Ptr> Decoders;
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
    ArchivedDecodersStorage()
    {
      using namespace Formats::Archived;
      Decoders.push_back(CreateZipDecoder());
      Decoders.push_back(CreateRarDecoder());
      Decoders.push_back(CreateZXZipDecoder());
      Decoders.push_back(CreateSCLDecoder());
      Decoders.push_back(CreateTRDDecoder());
      Decoders.push_back(CreateHripDecoder());
      //Decoders.push_back(CreateAYDecoder());
      //Decoders.push_back(CreateLhaDecoder());
      Decoders.push_back(CreateZXStateDecoder());
    }

    virtual bool ForEach(Formats::Archived::DecodersStorage::Visitor& visitor) const
    {
      return Decoders.end() != std::find_if(Decoders.begin(), Decoders.end(),
        boost::bind(&Formats::Archived::DecodersStorage::Visitor::Accept, &visitor, _1));
    }
  private:
    std::vector<Formats::Archived::Decoder::Ptr> Decoders;
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
  class Result
  {
  public:
    typedef boost::shared_ptr<const Result> Ptr;
    virtual ~Result() {}

    virtual String Name() const = 0;
    virtual Binary::Container::Ptr Data() const = 0;
  };

  Result::Ptr CreateResult(const String& name, Binary::Container::Ptr data);

  typedef DataReceiver<Result::Ptr> Target;

  typedef DataTransmitter<Result::Ptr> Source;
  typedef DataTransceiver<Result::Ptr> Pipe;
}

namespace
{
  class StaticResult : public Parsing::Result
  {
  public:
    StaticResult(const String& name, Binary::Container::Ptr data)
      : NameVal(name)
      , DataVal(data)
    {
    }

    virtual String Name() const
    {
      return NameVal;
    }

    virtual Binary::Container::Ptr Data() const
    {
      return DataVal;
    }
  private:
    const String NameVal;
    const Binary::Container::Ptr DataVal;
  };
}

namespace Parsing
{
  Result::Ptr CreateResult(const String& name, Binary::Container::Ptr data)
  {
    return boost::make_shared<StaticResult>(name, data);
  }
}

namespace
{
  class SaveTarget : public Parsing::Target
  {
  public:
    SaveTarget()
      : Params(Parameters::Container::Create())
    {
      Params->SetValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, 2);
    }

    virtual void ApplyData(const Parsing::Result::Ptr& result)
    {
      try
      {
        const String filePath = result->Name();
        const Binary::OutputStream::Ptr target = IO::CreateStream(filePath, *Params, Log::ProgressCallback::Stub());
        const Binary::Container::Ptr data = result->Data();
        target->ApplyData(*data);
      }
      catch (const Error& e)
      {
        std::cout << e.ToString();
      }
    }

    virtual void Flush()
    {
    }
  private:
    const Parameters::Container::Ptr Params;
  };

  class StatisticTarget : public Parsing::Target
  {
  public:
    StatisticTarget()
      : Total(0)
      , TotalSize(0)
    {
    }

    virtual void ApplyData(const Parsing::Result::Ptr& data)
    {
      ++Total;
      TotalSize += data->Data()->Size();
    }

    virtual void Flush()
    {
      std::cout << Strings::Format(Text::STATISTIC_OUTPUT, Total, TotalSize) << std::endl;
    }
  private:
    std::size_t Total;
    uint64_t TotalSize;
  };
}

namespace Parsing
{
  Parsing::Target::Ptr CreateSaveTarget()
  {
    return boost::make_shared<SaveTarget>();
  }

  Parsing::Target::Ptr CreateStatisticTarget()
  {
    return boost::make_shared<StatisticTarget>();
  }
}

namespace
{
  class SizeFilter : public Analysis::NodeReceiver
  {
  public:
    SizeFilter(std::size_t minSize, Analysis::NodeReceiver::Ptr target)
      : MinSize(minSize)
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& result)
    {
      const Binary::Container::Ptr data = result->Data();
      if (data->Size() >= MinSize)
      {
        Target->ApplyData(result);
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const std::size_t MinSize;
    const Analysis::NodeReceiver::Ptr Target;
  };

  class EmptyDataFilter : public Analysis::NodeReceiver
  {
  public:
    explicit EmptyDataFilter(Analysis::NodeReceiver::Ptr target)
      : Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& result)
    {
      const Binary::Container::Ptr data = result->Data();
      const uint8_t* const begin = static_cast<const uint8_t*>(data->Start());
      const uint8_t* const end = begin + data->Size();
      if (end != std::find_if(begin, end, std::bind1st(std::not_equal_to<uint8_t>(), *begin)))
      {
        Target->ApplyData(result);
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Analysis::NodeReceiver::Ptr Target;
  };

  class MatchedDataFilter : public Analysis::NodeReceiver
  {
  public:
    MatchedDataFilter(const std::string& format, Analysis::NodeReceiver::Ptr target)
      : Format(Binary::Format::Create(format))
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& result)
    {
      const Binary::Container::Ptr data = result->Data();
      const std::size_t size = data->Size();
      if (size != Format->Search(*data))
      {
        Target->ApplyData(result);
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Binary::Format::Ptr Format;
    const Analysis::NodeReceiver::Ptr Target;
  };
}

namespace Analysis
{
  NodeReceiver::Ptr CreateSizeFilter(std::size_t minSize, NodeReceiver::Ptr target)
  {
    return boost::make_shared<SizeFilter>(minSize, target);
  }

  NodeReceiver::Ptr CreateEmptyDataFilter(NodeReceiver::Ptr target)
  {
    return boost::make_shared<EmptyDataFilter>(target);
  }

  NodeReceiver::Ptr CreateMatchFilter(const std::string& filter, NodeReceiver::Ptr target)
  {
    return boost::make_shared<MatchedDataFilter>(filter, target);
  }
}

namespace
{
  typedef ObjectsStorage<Analysis::Service::Ptr> AnalyseServicesStorage;

  class AnalysedDataDispatcher : public AnalyseServicesStorage
                               , public Analysis::NodeTransmitter
  {
  public:
    typedef boost::shared_ptr<AnalysedDataDispatcher> Ptr;
  };

  class PackedDataAnalyseService : public Analysis::Service
  {
  public:
    PackedDataAnalyseService(Formats::Packed::Decoder::Ptr decoder, Analysis::NodeReceiver::Ptr unpacked)
      : Decoder(decoder)
      , Unpacked(unpacked)
    {
    }

    virtual Analysis::Result::Ptr Analyse(Analysis::Node::Ptr node) const
    {
      const Binary::Container::Ptr rawData = node->Data();
      const String descr = Decoder->GetDescription();
      Dbg("Trying '%1%'", descr);
      if (Formats::Packed::Container::Ptr depacked = Decoder->Decode(*rawData))
      {
        const std::size_t matched = depacked->PackedSize();
        Dbg("Found in %1% bytes", matched);
        const String name = descr;//TODO
        const Analysis::Node::Ptr subNode = Analysis::CreateSubnode(node, depacked, name, descr);
        Unpacked->ApplyData(subNode);
        return Analysis::CreateMatchedResult(matched);
      }
      else
      {
        Dbg("Not found");
        return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
      }
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Analysis::NodeReceiver::Ptr Unpacked;
  };

  class PackedDataAdapter : public Formats::Packed::DecodersStorage::Visitor
  {
  public:
    PackedDataAdapter(AnalyseServicesStorage::Visitor& delegate, Analysis::NodeReceiver::Ptr unpacked)
      : Delegate(delegate)
      , Unpacked(unpacked)
    {
    }

    virtual bool Accept(Formats::Packed::Decoder::Ptr decoder)
    {
      const Analysis::Service::Ptr service = boost::make_shared<PackedDataAnalyseService>(decoder, Unpacked);
      return Delegate.Accept(service);
    }
  private:
    AnalyseServicesStorage::Visitor& Delegate;
    const Analysis::NodeReceiver::Ptr Unpacked;
  };

  class PackedDataAnalysersStorage : public AnalysedDataDispatcher
  {
  public:
    PackedDataAnalysersStorage()
      : Decoders(Formats::Packed::GetAvailableDecoders())
      , Unpacked(Analysis::NodeReceiver::CreateStub())
    {
    }

    virtual bool ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      PackedDataAdapter adapter(visitor, Unpacked);
      return Decoders->ForEach(adapter);
    }
    
    virtual void SetTarget(Analysis::NodeReceiver::Ptr unpacked)
    {
      assert(unpacked);
      Unpacked = unpacked;
    }
  private:
    const Formats::Packed::DecodersStorage::Ptr Decoders;
    Analysis::NodeReceiver::Ptr Unpacked;
  };

  class ArchivedDataAnalyseService : public Analysis::Service
  {
  public:
    ArchivedDataAnalyseService(Formats::Archived::Decoder::Ptr decoder, Analysis::NodeReceiver::Ptr unarchived)
      : Decoder(decoder)
      , Unarchived(unarchived)
    {
    }

    virtual Analysis::Result::Ptr Analyse(Analysis::Node::Ptr node) const
    {
      const Binary::Container::Ptr rawData = node->Data();
      const String descr = Decoder->GetDescription();
      Dbg("Trying '%1%'", descr);
      if (Formats::Archived::Container::Ptr depacked = Decoder->Decode(*rawData))
      {
        const std::size_t matched = depacked->Size();
        Dbg("Found in %1% bytes", matched);
        const String name = descr;//TODO
        const Analysis::Node::Ptr subNode = Analysis::CreateSubnode(node, depacked, name, descr);
        const DepackFiles walker(subNode, Unarchived);
        depacked->ExploreFiles(walker);
        return Analysis::CreateMatchedResult(matched);
      }
      else
      {
        Dbg("Not found");
        return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
      }
    }
  private:
    class DepackFiles : public Formats::Archived::Container::Walker
    {
    public:
      DepackFiles(Analysis::Node::Ptr node, Analysis::NodeReceiver::Ptr unarchived)
        : ArchiveNode(node)
        , Unarchived(unarchived)
      {
      }

      virtual void OnFile(const Formats::Archived::File& file) const
      {
        if (Binary::Container::Ptr data = file.GetData())
        {
          const String name = file.GetName();
          const Analysis::Node::Ptr subNode = Analysis::CreateSubnode(ArchiveNode, data, name);
          Unarchived->ApplyData(subNode);
        }
      }
    private:
      const Analysis::Node::Ptr ArchiveNode;
      const Analysis::NodeReceiver::Ptr Unarchived;
    };
  private:
    const Formats::Archived::Decoder::Ptr Decoder;
    const Analysis::NodeReceiver::Ptr Unarchived;
  };

  class ArchivedDataAdapter : public Formats::Archived::DecodersStorage::Visitor
  {
  public:
    ArchivedDataAdapter(AnalyseServicesStorage::Visitor& delegate, Analysis::NodeReceiver::Ptr unarchived)
      : Delegate(delegate)
      , Unarchived(unarchived)
    {
    }

    virtual bool Accept(Formats::Archived::Decoder::Ptr decoder)
    {
      const Analysis::Service::Ptr service = boost::make_shared<ArchivedDataAnalyseService>(decoder, Unarchived);
      return Delegate.Accept(service);
    }
  private:
    AnalyseServicesStorage::Visitor& Delegate;
    const Analysis::NodeReceiver::Ptr Unarchived;
  };

  class ArchivedDataAnalysersStorage : public AnalysedDataDispatcher
  {
  public:
    ArchivedDataAnalysersStorage()
      : Decoders(Formats::Archived::GetAvailableDecoders())
      , Unarchived(Analysis::NodeReceiver::CreateStub())
    {
    }

    virtual bool ForEach(AnalyseServicesStorage::Visitor& visitor) const
    {
      ArchivedDataAdapter adapter(visitor, Unarchived);
      return Decoders->ForEach(adapter);
    }

    virtual void SetTarget(Analysis::NodeReceiver::Ptr unarchived)
    {
      assert(unarchived);
      Unarchived = unarchived;
    }
  private:
    const Formats::Archived::DecodersStorage::Ptr Decoders;
    Analysis::NodeReceiver::Ptr Unarchived;
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

  typedef boost::shared_ptr<const std::size_t> OffsetPtr;
  typedef boost::shared_ptr<std::size_t> OffsetRWPtr;

  class ScanningNode : public Analysis::Node
  {
  public:
    ScanningNode(Analysis::Node::Ptr parent, OffsetPtr offset, OffsetPtr limit)
      : ParentVal(parent)
      , DataVal(parent->Data())
      , Offset(offset)
      , Limit(limit)
    {
    }

    virtual String Name() const
    {
      //TODO
      return Strings::Format("+%1%", *Offset);
    }

    virtual Binary::Container::Ptr Data() const
    {
      const std::size_t offset = *Offset;
      const std::size_t available = *Limit;
      Require(available > offset);
      return DataVal->GetSubcontainer(offset, available - offset);
    }

    virtual String Provider() const
    {
      //TODO
      return String();
    }

    virtual Analysis::Node::Ptr Parent() const
    {
      return ParentVal;
    }
  private:
    const Analysis::Node::Ptr ParentVal;
    const Binary::Container::Ptr DataVal;
    const OffsetPtr Offset;
    const OffsetPtr Limit;
  };

  class ScanningDataAnalyser : private AnalyseServicesStorage::Visitor
  {
  public:
    ScanningDataAnalyser(Analysis::Node::Ptr source, AnalyseServicesStorage::Ptr services, Analysis::NodeReceiver::Ptr unresolved)
      : Source(source)
      , Services(services)
      , Unresolved(unresolved)
      , Limit(source->Data()->Size())
    {
    }

    void Analyse()
    {
      UnresolvedOffset = boost::make_shared<std::size_t>(0);
      ResetScaner(0);
      Services->ForEach(*this);
      while (*ScanOffset < *ScanLimit)
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
      ProcessUnresolved();
    }
  private:
    virtual bool Accept(Analysis::Service::Ptr service)
    {
      const Analysis::Result::Ptr result = service->Analyse(ScanNode);
      if (const std::size_t matched = result->GetMatchedDataSize())
      {
        const std::size_t nextOffset = *ScanOffset + matched;
        Dbg("Matched at %1%..%2%", *ScanOffset, nextOffset);
        //limit result for captured ScanNode
        *ScanLimit = nextOffset;

        ProcessUnresolved();
        ResetScaner(nextOffset);
        if (nextOffset == Limit)
        {
          Dbg("No more data to analyze. Stop.");
          return true;
        }
        Analysers.insert(std::make_pair(nextOffset, service));
      }
      else
      {
        const std::size_t lookahead = result->GetLookaheadOffset();
        const std::size_t nextSearch = *ScanOffset + std::max(lookahead, std::size_t(1));
        if (nextSearch < Limit)
        {
          Dbg("Skip for nearest %1% bytes", lookahead);
          Analysers.insert(std::make_pair(nextSearch, service));
        }
        else
        {
          Dbg("Disable for further checking");
        }
      }
      return false;
    }

    void ProcessUnresolved()
    {
      if (*UnresolvedOffset < *ScanOffset)
      {
        Dbg("Unresolved %1%..%2%", *UnresolvedOffset, *ScanOffset);
        const Analysis::Node::Ptr unresolvedNode = boost::make_shared<ScanningNode>(Source, UnresolvedOffset, ScanOffset);
        Unresolved->ApplyData(unresolvedNode);
        //recreate
        UnresolvedOffset = boost::make_shared<std::size_t>(*ScanOffset);
      }
    }

    void ResetScaner(std::size_t offset)
    {
      //recreate
      ScanOffset = boost::make_shared<std::size_t>(offset);
      ScanLimit = boost::make_shared<std::size_t>(Limit);
      ScanNode = boost::make_shared<ScanningNode>(Source, ScanOffset, ScanLimit);
      *UnresolvedOffset = offset;
    }
  private:
    const Analysis::Node::Ptr Source;
    const AnalyseServicesStorage::Ptr Services;
    const Analysis::NodeReceiver::Ptr Unresolved;
    const std::size_t Limit;
    OffsetRWPtr UnresolvedOffset;
    OffsetRWPtr ScanOffset;
    OffsetRWPtr ScanLimit;
    Analysis::Node::Ptr ScanNode;

    typedef std::multimap<std::size_t, Analysis::Service::Ptr> AnalysersCache;
    AnalysersCache Analysers;
  };

  class UnresolvedData : public Analysis::NodeTransceiver
  {
  public:
    explicit UnresolvedData(AnalyseServicesStorage::Ptr services)
      : Services(services)
      , Unresolved(Analysis::NodeReceiver::CreateStub())
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& node)
    {
      ScanningDataAnalyser analyser(node, Services, Unresolved);
      analyser.Analyse();
    }

    virtual void Flush()
    {
      Unresolved->Flush();
    }

    virtual void SetTarget(Analysis::NodeReceiver::Ptr unresolved)
    {
      assert(unresolved);
      Unresolved = unresolved;
    }
  private:
    const AnalyseServicesStorage::Ptr Services;
    Analysis::NodeReceiver::Ptr Unresolved;
  };

  typedef DataReceiver<String> StringsReceiver;

  typedef DataTransceiver<String, Analysis::Node::Ptr> OpenPoint;

  class OpenPointImpl : public OpenPoint
  {
  public:
    OpenPointImpl()
      : Analyse(Analysis::NodeReceiver::CreateStub())
      , Params(Parameters::Container::Create())
    {
    }

    virtual void ApplyData(const String& filename)
    {
      try
      {
        Dbg("Opening '%1%'", filename);
        const Binary::Container::Ptr data = IO::OpenData(filename, *Params, Log::ProgressCallback::Stub());
        const Analysis::Node::Ptr root = Analysis::CreateRootNode(data, filename);
        Analyse->ApplyData(root);
      }
      catch (const Error& e)
      {
        std::cout << e.ToString();
      }
    }

    virtual void Flush()
    {
      Analyse->Flush();
    }

    virtual void SetTarget(Analysis::NodeReceiver::Ptr analyse)
    {
      assert(analyse);
      Analyse = analyse;
    }
  private:
    Analysis::NodeReceiver::Ptr Analyse;
    const Parameters::Accessor::Ptr Params;
  };

  class PathTemplate : public Strings::FieldsSource
  {
  public:
    explicit PathTemplate(Analysis::Node::Ptr node)
      : Node(node)
    {
    }

    virtual String GetFieldValue(const String& fieldName) const
    {
      static const Char SUBPATH_DELIMITER[] = {'/', 0};
      static const Char FLATPATH_DELIMITER[] = {'_', 0};

      if (fieldName == Text::TEMPLATE_FIELD_FILENAME)
      {
        const IO::Identifier& id = GetRootIdentifier();
        return id.Filename();
      }
      else if (fieldName == Text::TEMPLATE_FIELD_PATH)
      {
        const IO::Identifier& id = GetRootIdentifier();
        return id.Path();
      }
      else if (fieldName == Text::TEMPLATE_FIELD_FLATPATH)
      {
        const IO::Identifier& id = GetRootIdentifier();
        const boost::filesystem::path path(id.Path());
        Strings::Array components;
        for (boost::filesystem::path::const_iterator it = path.begin(), lim = path.end(); it != lim; ++it)
        {
          components.push_back(it->string());
        }
        return boost::algorithm::join(components, FLATPATH_DELIMITER);
      }
      else if (fieldName == Text::TEMPLATE_FIELD_SUBPATH)
      {
        const Strings::Array& subPath = GetSubpath();
        return boost::algorithm::join(subPath, SUBPATH_DELIMITER);
      }
      else if (fieldName == Text::TEMPLATE_FIELD_FLATSUBPATH)
      {
        const Strings::Array& subPath = GetSubpath();
        return boost::algorithm::join(subPath, FLATPATH_DELIMITER);
      }
      else
      {
        return String();
      }
    }
  private:
    const IO::Identifier& GetRootIdentifier() const
    {
      if (!RootIdentifier)
      {
        FillCache();
      }
      return *RootIdentifier;
    }

    const Strings::Array& GetSubpath() const
    {
      if (!Subpath.get())
      {
        FillCache();
      }
      return *Subpath;
    }

    void FillCache() const
    {
      assert(!RootNode && !Subpath.get());
      Strings::Array subpath;
      for (Analysis::Node::Ptr node = Node; node;)
      {
        if (const Analysis::Node::Ptr prevNode = node->Parent())
        {
          subpath.push_back(node->Name());
          node = prevNode;
        }
        else
        {
          const String fileName = node->Name();
          RootIdentifier = IO::ResolveUri(fileName);
          Subpath.reset(new Strings::Array(subpath.rbegin(), subpath.rend()));
          break;
        }
      }
    }

  private:
    const Analysis::Node::Ptr Node;
    mutable IO::Identifier::Ptr RootIdentifier;
    mutable std::auto_ptr<Strings::Array> Subpath;
  };

  class TargetNamePoint : public Analysis::NodeReceiver
  {
  public:
    TargetNamePoint(const String& nameTemplate, Parsing::Target::Ptr target)
      : Template(Strings::Template::Create(nameTemplate))
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& node)
    {
      const PathTemplate fields(node);
      const String filename = Template->Instantiate(fields);
      const Parsing::Result::Ptr result = Parsing::CreateResult(filename, node->Data());
      Target->ApplyData(result);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Strings::Template::Ptr Template;
    const Parsing::Target::Ptr Target;
  };

  class Valve : public Analysis::NodeTransceiver
  {
  public:
    explicit Valve(Analysis::NodeReceiver::Ptr target = Analysis::NodeReceiver::CreateStub())
      : Target(target)
    {
    }

    virtual ~Valve()
    {
      //break possible cycles
      Target = Analysis::NodeReceiver::CreateStub();
    }

    virtual void ApplyData(const Analysis::Node::Ptr& data)
    {
      Target->ApplyData(data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(Analysis::NodeReceiver::Ptr target)
    {
      assert(target);
      Target = target;
    }
  private:
    Analysis::NodeReceiver::Ptr Target;
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
      if (boost::filesystem::is_directory(path))
      {
        ApplyRecursive(path);
      }
      else
      {
        Target->ApplyData(filename);
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
    void ApplyRecursive(const boost::filesystem::path& path) const
    {
      for (boost::filesystem::recursive_directory_iterator iter(path/*, boost::filesystem::symlink_option::recurse*/), lim = boost::filesystem::recursive_directory_iterator();
           iter != lim; ++iter)
      {
        const boost::filesystem::path subpath = iter->path();
        if (!boost::filesystem::is_directory(iter->status()))
        {
          const String subPathString = subpath.string();
          Target->ApplyData(subPathString);
        }
      }
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

    virtual String TargetNameTemplate() const = 0;
    virtual bool IgnoreEmptyData() const = 0;
    virtual std::size_t MinDataSize() const = 0;
    virtual std::string FormatFilter() const = 0;
    virtual std::size_t SaveThreadsCount() const = 0;
    virtual std::size_t SaveDataQueueSize() const = 0;
    virtual bool StatisticOutput() const = 0;
  };

  class AnalysisOptions
  {
  public:
    virtual ~AnalysisOptions() {}

    virtual std::size_t AnalysisThreads() const = 0;
    virtual std::size_t AnalysisDataQueueSize() const = 0;
  };

  Analysis::NodeReceiver::Ptr CreateTarget(const TargetOptions& opts)
  {
    const Parsing::Target::Ptr save = opts.StatisticOutput()
      ? Parsing::CreateStatisticTarget()
      : Parsing::CreateSaveTarget();
    const Analysis::NodeReceiver::Ptr makeName = boost::make_shared<TargetNamePoint>(opts.TargetNameTemplate(), save);
    const Analysis::NodeReceiver::Ptr storeAll = makeName;
    const Analysis::NodeReceiver::Ptr storeNoEmpty = opts.IgnoreEmptyData()
      ? Analysis::CreateEmptyDataFilter(storeAll)
      : storeAll;
    const std::size_t minSize = opts.MinDataSize();
    const Analysis::NodeReceiver::Ptr storeEnoughSize = minSize
      ? Analysis::CreateSizeFilter(minSize, storeNoEmpty)
      : storeNoEmpty;
    const std::string filter = opts.FormatFilter();
    const Analysis::NodeReceiver::Ptr storeMatchedFilter = !filter.empty()
      ? Analysis::CreateMatchFilter(filter, storeEnoughSize)
      : storeEnoughSize;
    const Analysis::NodeReceiver::Ptr result = storeMatchedFilter;
    return AsyncWrap<Analysis::Node::Ptr>(opts.SaveThreadsCount(), opts.SaveDataQueueSize(), result);
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

  Analysis::NodeTransceiver::Ptr CreateAnalyser(const AnalysisOptions& opts)
  {
    //form analysers
    const AnalysedDataDispatcher::Ptr unarchive = boost::make_shared<ArchivedDataAnalysersStorage>();
    const AnalysedDataDispatcher::Ptr depack = boost::make_shared<PackedDataAnalysersStorage>();
    const AnalyseServicesStorage::Ptr anyConversion = boost::make_shared<CompositeAnalyseServicesStorage>(unarchive, depack);
    const Analysis::NodeTransceiver::Ptr filterUnresolved = boost::make_shared<UnresolvedData>(anyConversion);

    const Analysis::NodeTransceiver::Ptr unknownData = boost::make_shared<Valve>();
    unknownData->SetTarget(filterUnresolved);
    unarchive->SetTarget(unknownData);
    depack->SetTarget(unknownData);
    const Analysis::NodeReceiver::Ptr input = AsyncWrap<Analysis::Node::Ptr>(opts.AnalysisThreads(), opts.AnalysisDataQueueSize(), unknownData);
    return boost::make_shared<TransceivePipe<Analysis::Node::Ptr> >(input, filterUnresolved);
  }

  OpenPoint::Ptr CreateSource()
  {
    const OpenPoint::Ptr open = boost::make_shared<OpenPointImpl>();
    const boost::shared_ptr<ResolveDirsPoint> resolve = boost::make_shared<ResolveDirsPoint>();
    resolve->SetTarget(open);
    return boost::make_shared<TransceivePipe<String, Analysis::Node::Ptr> >(resolve, open);
  }

  class Options : public AnalysisOptions
                , public TargetOptions
  {
  public:
    Options()
      : AnalysisThreadsValue(1)
      , AnalysisDataQueueSizeValue(10)
      , TargetNameTemplateValue(Text::DEFAULT_TARGET_NAME_TEMPLATE)
      , IgnoreEmptyDataValue(false)
      , MinDataSizeValue(0)
      , FormatFilterValue()
      , SaveThreadsCountValue(1)
      , SaveDataQueueSizeValue(500)
      , StatisticOutputValue(false)
      //cmdline
      , OptionsDescription(Text::TARGET_SECTION)
    {
      using namespace boost::program_options;
      OptionsDescription.add_options()
        (Text::ANALYSIS_THREADS_KEY, value<std::size_t>(&AnalysisThreadsValue), Text::ANALYSIS_THREADS_DESC)
        (Text::ANALYSIS_QUEUE_SIZE_KEY, value<std::size_t>(&AnalysisDataQueueSizeValue), Text::ANALYSIS_QUEUE_SIZE_DESC)
        (Text::TARGET_NAME_TEMPLATE_KEY, value<String>(&TargetNameTemplateValue), Text::TARGET_NAME_TEMPLATE_DESC)
        (Text::IGNORE_EMPTY_KEY, bool_switch(&IgnoreEmptyDataValue), Text::IGNORE_EMPTY_DESC)
        (Text::MINIMAL_SIZE_KEY, value<std::size_t>(&MinDataSizeValue), Text::MINIMAL_SIZE_DESC)
        (Text::FORMAT_FILTER_KEY, value<std::string>(&FormatFilterValue), Text::FORMAT_FILTER_DESC)
        (Text::SAVE_THREADS_KEY, value<std::size_t>(&SaveThreadsCountValue), Text::SAVE_THREADS_DESC)
        (Text::SAVE_QUEUE_SIZE_KEY, value<std::size_t>(&SaveDataQueueSizeValue), Text::SAVE_QUEUE_SIZE_DESC)
        (Text::OUTPUT_STATISTIC_KEY, bool_switch(&StatisticOutputValue), Text::OUTPUT_STATISTIC_DESC)
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

    virtual String TargetNameTemplate() const
    {
      return TargetNameTemplateValue;
    }

    virtual bool IgnoreEmptyData() const
    {
      return IgnoreEmptyDataValue;
    }

    virtual std::size_t MinDataSize() const
    {
      return MinDataSizeValue;
    }

    virtual std::string FormatFilter() const
    {
      return FormatFilterValue;
    }

    virtual std::size_t SaveThreadsCount() const
    {
      return SaveThreadsCountValue;
    }

    virtual std::size_t SaveDataQueueSize() const
    {
      return SaveDataQueueSizeValue;
    }

    virtual bool StatisticOutput() const
    {
      return StatisticOutputValue;
    }

    const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
  private:
    std::size_t AnalysisThreadsValue;
    std::size_t AnalysisDataQueueSizeValue;
    String TargetNameTemplateValue;
    bool IgnoreEmptyDataValue;
    std::size_t MinDataSizeValue;
    std::string FormatFilterValue;
    std::size_t SaveThreadsCountValue;
    std::size_t SaveDataQueueSizeValue;
    bool StatisticOutputValue;
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
    std::locale::global(std::locale(""));
    /*

                                           analyseThreads                 saveThreads
                                             |                              |
    file/dir -> [resolve] -> name -> [open] -*-> data -> [convert] -> data -*-> [filter] -> [createDir] -> [save]
                                        |           |            |                             |       |
                                       factory      +<-converted-+                            factory directory

    */

    const Options opts;
    Strings::Array paths;
    {
      using namespace boost::program_options;
      options_description options(Strings::Format(Text::USAGE_SECTION, *argv));
      options.add_options()
        (Text::HELP_KEY, Text::HELP_DESC)
        (Text::VERSION_KEY, Text::VERSION_DESC)
      ;
      options.add(opts.GetOptionsDescription());
      options.add_options()
        (Text::INPUT_KEY, value<Strings::Array>(&paths), Text::INPUT_DESC)
      ;
      positional_options_description inputPositional;
      inputPositional.add(Text::INPUT_KEY, -1);

      variables_map vars;
      store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
      notify(vars);

      if (vars.count(Text::VERSION_KEY))
      {
        std::cout << GetProgramVersionString() << std::endl;
        return true;
      }
      else if (vars.count(Text::HELP_KEY) || paths.empty())
      {
        std::cout << options << std::endl;
        return true;
      }
    }

    const Analysis::NodeReceiver::Ptr result = CreateTarget(opts);
    const Analysis::NodeTransceiver::Ptr analyse = CreateAnalyser(opts);
    const OpenPoint::Ptr input = CreateSource();

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
    std::cout << e.ToString();
  }
}
