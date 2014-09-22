/**
* 
* @file
*
* @brief XTractor tool main file
*
* @author vitamin.caig@gmail.com
*
**/


//local includes
#include <apps/version/api.h>
//common includes
#include <progress_callback.h>
//library includes
#include <analysis/path.h>
#include <analysis/result.h>
#include <analysis/scanner.h>
#include <async/data_receiver.h>
#include <binary/format_factories.h>
#include <debug/log.h>
#include <formats/archived/decoders.h>
#include <formats/chiptune/decoders.h>
#include <formats/image/decoders.h>
#include <formats/packed/decoders.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <io/impl/boost_filesystem_path.h>
#include <parameters/container.h>
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
    //! Parent node. Ptr() if root node
    virtual Ptr Parent() const = 0;
  };

  Node::Ptr CreateRootNode(Binary::Container::Ptr data, const String& name);
  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name);
  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, std::size_t offset);
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
    SubNode(Analysis::Node::Ptr parent, Binary::Container::Ptr data, const String& name)
      : ParentVal(parent)
      , DataVal(data)
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

    virtual Analysis::Node::Ptr Parent() const
    {
      return ParentVal;
    }
  private:
    const Analysis::Node::Ptr ParentVal;
    const Binary::Container::Ptr DataVal;
    const String NameVal;
  };
}

namespace Analysis
{
  //since data is required, place it first
  Node::Ptr CreateRootNode(Binary::Container::Ptr data, const String& name)
  {
    return boost::make_shared<RootNode>(data, name);
  }

  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name)
  {
    return boost::make_shared<SubNode>(parent, data, name);
  }

  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, std::size_t offset)
  {
    return boost::make_shared<SubNode>(parent, data, Strings::Format("+%1%", offset));
  }

  Node::Ptr CreateSubnode(Node::Ptr parent, Binary::Container::Ptr data, const String& name, std::size_t offset)
  {
    const Node::Ptr intermediate = CreateSubnode(parent, data, offset);
    return CreateSubnode(intermediate, data, name);
  }
}

namespace Analysis
{
  typedef DataReceiver<Node::Ptr> NodeReceiver;
  typedef DataTransmitter<Node::Ptr> NodeTransmitter;
  typedef DataTransceiver<Node::Ptr> NodeTransceiver;
}

namespace Formats
{
  namespace Archived
  { 
    void FillScanner(Analysis::Scanner& scanner)
    {
      scanner.AddDecoder(CreateZipDecoder());
      scanner.AddDecoder(CreateRarDecoder());
      scanner.AddDecoder(CreateZXZipDecoder());
      scanner.AddDecoder(CreateSCLDecoder());
      scanner.AddDecoder(CreateTRDDecoder());
      scanner.AddDecoder(CreateHripDecoder());
      scanner.AddDecoder(CreateAYDecoder());
      scanner.AddDecoder(CreateLhaDecoder());
      scanner.AddDecoder(CreateZXStateDecoder());
    }
  }

  namespace Packed
  {
    void FillScanner(Analysis::Scanner& scanner)
    {
      scanner.AddDecoder(CreateCodeCruncher3Decoder());
      scanner.AddDecoder(CreateCompressorCode4Decoder());
      scanner.AddDecoder(CreateCompressorCode4PlusDecoder());
      scanner.AddDecoder(CreateDataSquieezerDecoder());
      scanner.AddDecoder(CreateESVCruncherDecoder());
      scanner.AddDecoder(CreateHrumDecoder());
      scanner.AddDecoder(CreateHrust1Decoder());
      scanner.AddDecoder(CreateHrust21Decoder());
      scanner.AddDecoder(CreateHrust23Decoder());
      scanner.AddDecoder(CreateLZSDecoder());
      scanner.AddDecoder(CreateMSPackDecoder());
      scanner.AddDecoder(CreatePowerfullCodeDecreaser61Decoder());
      scanner.AddDecoder(CreatePowerfullCodeDecreaser61iDecoder());
      scanner.AddDecoder(CreatePowerfullCodeDecreaser62Decoder());
      scanner.AddDecoder(CreateTRUSHDecoder());
      scanner.AddDecoder(CreateGamePackerDecoder());
      scanner.AddDecoder(CreateGamePackerPlusDecoder());
      scanner.AddDecoder(CreateTurboLZDecoder());
      scanner.AddDecoder(CreateTurboLZProtectedDecoder());
      scanner.AddDecoder(CreateCharPresDecoder());
      scanner.AddDecoder(CreatePack2Decoder());
      scanner.AddDecoder(CreateLZH1Decoder());
      scanner.AddDecoder(CreateLZH2Decoder());
      scanner.AddDecoder(CreateFullDiskImageDecoder());
      scanner.AddDecoder(CreateHobetaDecoder());
      scanner.AddDecoder(CreateSna128Decoder());
      scanner.AddDecoder(CreateTeleDiskImageDecoder());
      scanner.AddDecoder(CreateZ80V145Decoder());
      scanner.AddDecoder(CreateZ80V20Decoder());
      scanner.AddDecoder(CreateZ80V30Decoder());
      scanner.AddDecoder(CreateMegaLZDecoder());
      //players
      scanner.AddDecoder(CreateCompiledASC0Decoder());
      scanner.AddDecoder(CreateCompiledASC1Decoder());
      scanner.AddDecoder(CreateCompiledASC2Decoder());
      scanner.AddDecoder(CreateCompiledST3Decoder());
      scanner.AddDecoder(CreateCompiledSTP1Decoder());
      scanner.AddDecoder(CreateCompiledSTP2Decoder());
      scanner.AddDecoder(CreateCompiledPT24Decoder());
      scanner.AddDecoder(CreateCompiledPTU13Decoder());
    }
  }

  namespace Image
  { 
    void FillScanner(Analysis::Scanner& scanner)
    {
      scanner.AddDecoder(CreateLaserCompact52Decoder());
      scanner.AddDecoder(CreateASCScreenCrusherDecoder());
      scanner.AddDecoder(CreateLaserCompact40Decoder());
    }
  }

  namespace Chiptune
  {
    void FillScanner(Analysis::Scanner& scanner)
    {
      //try TurboSound first
      scanner.AddDecoder(CreateTurboSoundDecoder());
      scanner.AddDecoder(CreatePSGDecoder());
      scanner.AddDecoder(CreateDigitalStudioDecoder());
      scanner.AddDecoder(CreateSoundTrackerDecoder());
      scanner.AddDecoder(CreateSoundTrackerCompiledDecoder());
      scanner.AddDecoder(CreateSoundTracker3Decoder());
      scanner.AddDecoder(CreateSoundTrackerProCompiledDecoder());
      scanner.AddDecoder(CreateASCSoundMaster0xDecoder());
      scanner.AddDecoder(CreateASCSoundMaster1xDecoder());
      scanner.AddDecoder(CreateProTracker2Decoder());
      scanner.AddDecoder(CreateProTracker3Decoder());
      scanner.AddDecoder(CreateProSoundMakerCompiledDecoder());
      scanner.AddDecoder(CreateGlobalTrackerDecoder());
      scanner.AddDecoder(CreateProTracker1Decoder());
      scanner.AddDecoder(CreateVTXDecoder());
      scanner.AddDecoder(CreateYMDecoder());
      scanner.AddDecoder(CreateTFDDecoder());
      scanner.AddDecoder(CreateTFCDecoder());
      scanner.AddDecoder(CreateVortexTracker2Decoder());
      scanner.AddDecoder(CreateChipTrackerDecoder());
      scanner.AddDecoder(CreateSampleTrackerDecoder());
      scanner.AddDecoder(CreateProDigiTrackerDecoder());
      scanner.AddDecoder(CreateSQTrackerDecoder());
      scanner.AddDecoder(CreateProSoundCreatorDecoder());
      scanner.AddDecoder(CreateFastTrackerDecoder());
      scanner.AddDecoder(CreateETrackerDecoder());
      scanner.AddDecoder(CreateSQDigitalTrackerDecoder());
      scanner.AddDecoder(CreateTFMMusicMaker05Decoder());
      scanner.AddDecoder(CreateTFMMusicMaker13Decoder());
      scanner.AddDecoder(CreateDigitalMusicMakerDecoder());
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
      : Format(Binary::CreateFormat(format))
      , Target(target)
    {
    }

    virtual void ApplyData(const Analysis::Node::Ptr& result)
    {
      const Binary::Container::Ptr data = result->Data();
      const std::size_t size = data->Size();
      if (Format->Match(*data) || size != Format->NextMatchOffset(*data))
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
  class NestedScannerTarget : public Analysis::Scanner::Target
  {
  public:
    NestedScannerTarget(Analysis::Node::Ptr root, Analysis::NodeReceiver& toScan, Analysis::NodeReceiver& toStore)
      : Root(root)
      , ToScan(toScan)
      , ToStore(toStore)
    {
    }

    virtual void Apply(const Formats::Archived::Decoder& decoder, std::size_t offset, Formats::Archived::Container::Ptr data)
    {
      const String name = decoder.GetDescription();
      Dbg("Found %1% in %2% bytes at %3%", name, data->Size(), offset);
      const Analysis::Node::Ptr archNode = Analysis::CreateSubnode(Root, data, name, offset);
      const ScanFiles walker(ToScan, archNode);
      data->ExploreFiles(walker);
    }

    virtual void Apply(const Formats::Packed::Decoder& decoder, std::size_t offset, Formats::Packed::Container::Ptr data)
    {
      const String name = decoder.GetDescription();
      Dbg("Found %1% in %2% bytes at %3%", name, data->PackedSize(), offset);
      const Analysis::Node::Ptr packNode = Analysis::CreateSubnode(Root, data, name, offset);
      ToScan.ApplyData(packNode);
    }

    virtual void Apply(const Formats::Image::Decoder& decoder, std::size_t offset, Formats::Image::Container::Ptr data)
    {
      const String name = decoder.GetDescription();
      Dbg("Found %1% in %2% bytes at %3%", name, data->OriginalSize(), offset);
      const Analysis::Node::Ptr imageNode = Analysis::CreateSubnode(Root, data, Strings::Format("+%1%.image", offset));
      ToStore.ApplyData(imageNode);
    }

    virtual void Apply(const Formats::Chiptune::Decoder& decoder, std::size_t offset, Formats::Chiptune::Container::Ptr data)
    {
      const String name = decoder.GetDescription();
      Dbg("Found %1% in %2% bytes at %3%", name, data->Size(), offset);
      const Analysis::Node::Ptr chiptuneNode = Analysis::CreateSubnode(Root, data, Strings::Format("+%1%.chiptune", offset));
      ToStore.ApplyData(chiptuneNode);
    }

    virtual void Apply(std::size_t offset, Binary::Container::Ptr data)
    {
      Dbg("Unresolved %1% bytes at %2%", data->Size(), offset);
      const Analysis::Node::Ptr rawNode = Analysis::CreateSubnode(Root, data, offset);
      ToStore.ApplyData(rawNode);
    }
  private:
    class ScanFiles : public Formats::Archived::Container::Walker
    {
    public:
      ScanFiles(Analysis::NodeReceiver& toScan, Analysis::Node::Ptr node)
        : ToScan(toScan)
        , ArchiveNode(node)
      {
      }

      virtual void OnFile(const Formats::Archived::File& file) const
      {
        if (const Binary::Container::Ptr data = file.GetData())
        {
          const String name = file.GetName();
          Dbg("Processing %1%", name);
          const Analysis::Node::Ptr fileNode = Analysis::CreateSubnode(ArchiveNode, data, name);
          ToScan.ApplyData(fileNode);
        }
      }
    private:
      Analysis::NodeReceiver& ToScan;
      const Analysis::Node::Ptr ArchiveNode;
    };
  private:
    const Analysis::Node::Ptr Root;
    Analysis::NodeReceiver& ToScan;
    Analysis::NodeReceiver& ToStore;
  };

  class AnalysisTarget : public Analysis::NodeTransceiver
  {
  public:
    AnalysisTarget()
      : Scanner(Analysis::CreateScanner())
    {
      Formats::Archived::FillScanner(*Scanner);
      Formats::Packed::FillScanner(*Scanner);
      Formats::Image::FillScanner(*Scanner);
      Formats::Chiptune::FillScanner(*Scanner);
    }

    virtual void ApplyData(const Analysis::Node::Ptr& node)
    {
      Dbg("Analyze %1%", node->Name());
      NestedScannerTarget target(node, *this, *Target);
      Scanner->Scan(node->Data(), target);
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void SetTarget(Analysis::NodeReceiver::Ptr target)
    {
      Target = target;
    }
  private:
    const Analysis::Scanner::RWPtr Scanner;
    Analysis::NodeReceiver::Ptr Target;
  };
}

namespace
{
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
        //TODO: use IO::FilenameTemplate
        const IO::Identifier& id = GetRootIdentifier();
        const boost::filesystem::path path(id.Path());
        const boost::filesystem::path root(path.root_directory());
        Strings::Array components;
        for (boost::filesystem::path::const_iterator it = path.begin(), lim = path.end(); it != lim; ++it)
        {
          if (*it != root)
          {
            components.push_back(IO::Details::ToString(*it));
          }
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
      assert(!RootIdentifier && !Subpath.get());
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
    const Analysis::NodeTransceiver::Ptr analyser = boost::make_shared<AnalysisTarget>();
    const Analysis::NodeReceiver::Ptr input = AsyncWrap<Analysis::Node::Ptr>(opts.AnalysisThreads(), opts.AnalysisDataQueueSize(), analyser);
    return boost::make_shared<TransceivePipe<Analysis::Node::Ptr> >(input, analyser);
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
