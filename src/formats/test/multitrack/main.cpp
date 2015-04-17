#include <formats/chiptune/multi/multitrackcontainer.h>
#include <binary/data_builder.h>
#include <core/module_attrs.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <boost/range/end.hpp>

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
  
  void Create(CmdlineIterator& arg)
  {
    const std::string& file = *arg++;
    if (file == "--help")
    {
      std::cout << "Create new container:\n"
                   " " << arg.Executable() << " --create <result> [file props] --track [track props] <file> [file props] [<file> [file props] ... ] [--track ...]\n"
                   "  where 'props' can be set of:\n"
                   "   --title <title>\n"
                   "   --author <author>\n"
                   "   --annotation <annotation>\n"
                   "   --property name=value\n"
      ;
      return;
    }
    const Formats::Chiptune::MultiTrackContainer::ContainerBuilder::Ptr builder = Formats::Chiptune::MultiTrackContainer::CreateBuilder();
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
        builder->SetProperty(Module::ATTR_FILENAME + "=" + cmd);
      }
    }
    WriteFile(*builder->GetResult(), file);
  }
  
  class Printer : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    Printer()
    {
    }
    
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
    if (file == "--help")
    {
      std::cout << "Show detailed content of container:\n"
                   " " << arg.Executable() << " --list <file>\n"
      ;
    }
    else
    {
      const Binary::Container::Ptr data = OpenFile(file);
      Printer printer;
      Formats::Chiptune::MultiTrackContainer::Parse(*data, printer);
    }
  }
  
  typedef void (*ModeFunc)(CmdlineIterator&);
  
  struct ModeEntry
  {
    const char* Name;
    ModeFunc Func;
  };
  
  const ModeEntry MODES[] =
  {
    {"--create", &Create},
    {"--list", &List}
  };
  
  void ListModes()
  {
    std::cout << "Use --<mode> --help to get specific information\n"
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
    for (const ModeEntry* it = MODES; it != boost::end(MODES); ++it)
    {
      if (mode == it->Name)
      {
        (it->Func)(++arg);
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
