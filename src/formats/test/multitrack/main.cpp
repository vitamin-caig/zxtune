//library includes
#include <binary/data_builder.h>
#include <core/module_attrs.h>
#include <formats/chiptune/multi/multitrackcontainer.h>
#include <platform/version/api.h>
#include <strings/format.h>
//std includes
#include <stdexcept>
#include <iostream>
#include <fstream>
//boost includes
#include <boost/range/end.hpp>

namespace Text
{
  extern const Char PROGRAM_NAME[] = {'m', 't', 'c', 't', 'o', 'o', 'l', 0};
}

namespace
{
  Binary::Container::Ptr OpenFile(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Binary::DataBuilder data(size);
    std::auto_ptr<Dump> tmp(new Dump(size));
    stream.read(static_cast<char*>(data.Allocate(size)), size);
    return data.CaptureResult();
  }
  
  void WriteFile(const Binary::Data& data, const std::string& name)
  {
    std::ofstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.write(static_cast<const char*>(data.Start()), data.Size());
  }
  
  class CmdlineIterator
  {
  public:
    CmdlineIterator(int argc, const char* argv[])
      : Argc(argc)
      , Argv(argv)
      , Pos()
    {
    }
    
    std::string Executable() const
    {
      return *Argv;
    }
    
    std::string operator * () const
    {
      CheckIsValid();
      return Argv[Pos];
    }
    
    CmdlineIterator& operator ++ ()
    {
      CheckIsValid();
      ++Pos;
      return *this;
    }
    
    CmdlineIterator operator ++ (int)
    {
      CmdlineIterator copy(*this);
      ++(*this);
      return copy;
    }
    
    typedef void (*BoolType)();
    operator BoolType () const
    {
      return IsValid() ? &std::abort : 0;
    }
  private:
    bool IsValid() const
    {
      return Pos < Argc;
    }
    
    void CheckIsValid() const
    {
      if (!IsValid())
      {
        throw std::runtime_error("Not enough parameters specified");
      }
    }
  private:
    const int Argc;
    const char** Argv;
    int Pos;
  };
  
  std::string GetFilename(const std::string& path)
  {
    const std::string::size_type delimPos = path.find_last_of("/\\");
    return delimPos == std::string::npos
         ? path
         : path.substr(delimPos + 1);
  }
  
  void Create(CmdlineIterator& arg)
  {
    const std::string& file = *arg++;
    const Formats::Chiptune::MultiTrackContainer::ContainerBuilder::Ptr builder = Formats::Chiptune::MultiTrackContainer::CreateBuilder();
    builder->SetProperty(Module::ATTR_PROGRAM + "=" + Platform::Version::GetProgramVersionString());
    uint_t track = 0;
    while (arg)
    {
      const std::string& cmd = *arg++;
      if (cmd == "--track")
      {
        builder->StartTrack(track++);
      }
      else if (cmd == "--title")
      {
        builder->SetTitle(*arg++);
      }
      else if (cmd == "--author")
      {
        builder->SetAuthor(*arg++);
      }
      else if (cmd == "--annotation")
      {
        builder->SetAnnotation(*arg++);
      }
      else if (cmd == "--property")
      {
        builder->SetProperty(*arg++);
      }
      else
      {
        const Binary::Container::Ptr data = OpenFile(cmd);
        builder->SetData(data);
        builder->SetProperty(Module::ATTR_FILENAME + "=" + GetFilename(cmd));
      }
    }
    WriteFile(*builder->GetResult(), file);
  }
  
  class Printer : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    virtual void SetAuthor(const String& author)
    {
      std::cout << Padding << "Author: " << author << std::endl;
    }
    
    virtual void SetTitle(const String& title)
    {
      std::cout << Padding << "Title: " << title << std::endl;
    }
    
    virtual void SetAnnotation(const String& annotation)
    {
      std::cout << Padding << "Annotation: " << annotation << std::endl;
    }

    virtual void SetProperty(const String& property)
    {
      std::cout << Padding << property << std::endl;
    }

    virtual void StartTrack(uint_t idx)
    {
      std::cout << " Track " << idx << std::endl;
      Padding.assign(2, ' ');
    }
   
    virtual void SetData(Binary::Container::Ptr data)
    {
      std::cout << Padding << "Data of size " << data->Size() << std::endl;
    }
  private:
    String Padding;
  };
  
  void List(CmdlineIterator& arg)
  {
    const std::string& file = *arg;
    const Binary::Container::Ptr data = OpenFile(file);
    Printer printer;
    Formats::Chiptune::MultiTrackContainer::Parse(*data, printer);
  }
  
  class Extractor : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    Extractor()
      : LastTrackIdx()
      , LastDataIdx()
    {
    }
    
    virtual void SetAuthor(const String& /*author*/) {}
    virtual void SetTitle(const String& /*title*/) {}
    virtual void SetAnnotation(const String& /*annotation*/) {}
    virtual void SetProperty(const String& property)
    {
      const String::size_type eqPos = property.find_first_of('=');
      const String name = property.substr(0, eqPos);
      const String value = eqPos != String::npos ? property.substr(eqPos + 1) : String();
      if (name == Module::ATTR_FILENAME)
      {
        LastDataName = value;
      }
    }

    virtual void StartTrack(uint_t idx)
    {
      Flush();
      LastTrackIdx = idx;
      LastDataIdx = 0;
    }
   
    virtual void SetData(Binary::Container::Ptr data)
    {
      Flush();
      LastData = data;
      ++LastDataIdx;
    }
    
    void Flush()
    {
      if (LastData)
      {
        const String& filename = LastDataName.empty()
                               ? Strings::Format("t%u_d%u", LastTrackIdx, LastDataIdx)
                               : LastDataName;
        std::cout << "Save " << LastData->Size() << " bytes to " << filename << std::endl;
        WriteFile(*LastData, filename);
        LastData.reset();
        LastDataName.clear();
      }
    }
  private:
    Binary::Container::Ptr LastData;
    String LastDataName;
    uint_t LastTrackIdx;
    uint_t LastDataIdx;
  };
  
  void Extract(CmdlineIterator& arg)
  {
    const std::string& file = *arg;
    const Binary::Container::Ptr data = OpenFile(file);
    Extractor extractor;
    Formats::Chiptune::MultiTrackContainer::Parse(*data, extractor);
    extractor.Flush();
  }
  
  typedef void (*ModeFunc)(CmdlineIterator&);
  
  struct ModeEntry
  {
    const char* Name;
    ModeFunc Func;
    const char* Help;
  };
  
  const ModeEntry MODES[] =
  {
    {
      "--create",
      &Create,
      "Create new container:\n"
      " mtctool --create <result> [file props] --track [track props] <file> [file props] [<file> [file props] ... ] [--track ...]\n"
      "  where 'props' can be set of:\n"
      "   --title <title>\n"
      "   --author <author>\n"
      "   --annotation <annotation>\n"
      "   --property name=value\n"
    },
    {
      "--list",
      &List,
      "Show detailed content of container:\n"
      " mtctool --list <file>\n"
    },
    {
      "--extract",
      &Extract,
      "Extract all the data to files in current dir:\n"
      " mtctool --extract <file>\n"
    }
  };
  
  void ListModes()
  {
    std::cout << "Use <mode> without other params to get specific information\n"
                 "Supported modes:" << std::endl;
    for (const ModeEntry* it = MODES; it != boost::end(MODES); ++it)
    {
      std::cout << " " << it->Name << std::endl;
    }
  }
}

int main(int argc, const char* argv[])
{
  try
  {
    if (argc < 2)
    {
      ListModes();
      return 1;
    }
    CmdlineIterator arg(argc, argv);
    const std::string mode = *++arg;
    ++arg;
    for (const ModeEntry* it = MODES; it != boost::end(MODES); ++it)
    {
      if (mode == it->Name)
      {
        if (arg)
        {
          (it->Func)(arg);
        }
        else
        {
          std::cout << it->Help;
        }
        return 0;
      }
    }
    throw std::runtime_error("Unsupported mode");
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
