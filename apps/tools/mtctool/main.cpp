// library includes
#include <binary/data_builder.h>
#include <formats/chiptune/multidevice/multitrackcontainer.h>
#include <module/attributes.h>
#include <platform/version/api.h>
#include <strings/format.h>
// common includes
#include <string_view.h>
// std includes
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace Platform::Version
{
  const StringView PROGRAM_NAME = "mtctool"sv;
}

namespace
{
  template<class Stream>
  Stream OpenStream(StringView name)
  {
    Stream result(String{name}.c_str(), std::ios::binary);
    if (!result)
    {
      throw std::runtime_error(String("Failed to open ").append(name));
    }
    return result;
  }

  Binary::Container::Ptr OpenFile(StringView name)
  {
    auto stream = OpenStream<std::ifstream>(name);
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Binary::DataBuilder data(size);
    stream.read(static_cast<char*>(data.Allocate(size)), size);
    return data.CaptureResult();
  }

  void WriteFile(Binary::View data, StringView name)
  {
    auto stream = OpenStream<std::ofstream>(name);
    stream.write(static_cast<const char*>(data.Start()), data.Size());
  }

  class CmdlineIterator
  {
  public:
    CmdlineIterator(int argc, const char* argv[])
      : Argc(argc)
      , Argv(argv)
    {}

    StringView Executable() const
    {
      return *Argv;
    }

    StringView operator*() const
    {
      CheckIsValid();
      return Argv[Pos];
    }

    CmdlineIterator& operator++()
    {
      CheckIsValid();
      ++Pos;
      return *this;
    }

    CmdlineIterator operator++(int)
    {
      CmdlineIterator copy(*this);
      ++(*this);
      return copy;
    }

    explicit operator bool() const
    {
      return IsValid();
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
    int Pos = 0;
  };

  StringView GetFilename(StringView path)
  {
    const auto delimPos = path.find_last_of("/\\");
    return delimPos == path.npos ? path : path.substr(delimPos + 1);
  }

  void Create(CmdlineIterator& arg)
  {
    const auto file = *arg++;
    const auto builder = Formats::Chiptune::MultiTrackContainer::CreateBuilder();
    builder->SetProperty(Module::ATTR_PROGRAM, Platform::Version::GetProgramVersionString());
    uint_t track = 0;
    while (arg)
    {
      const auto cmd = *arg++;
      if (cmd == "--track"sv)
      {
        builder->StartTrack(track++);
      }
      else if (cmd == "--title"sv)
      {
        builder->SetTitle(*arg++);
      }
      else if (cmd == "--author"sv)
      {
        builder->SetAuthor(*arg++);
      }
      else if (cmd == "--annotation"sv)
      {
        builder->SetAnnotation(*arg++);
      }
      else if (cmd == "--property"sv)
      {
        const auto name = *arg++;
        const auto value = *arg++;
        builder->SetProperty(name, value);
      }
      else
      {
        auto data = OpenFile(cmd);
        builder->SetData(std::move(data));
        builder->SetProperty(Module::ATTR_FILENAME, GetFilename(cmd));
      }
    }
    WriteFile(*builder->GetResult(), file);
  }

  class Printer : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    void SetAuthor(StringView author) override
    {
      std::cout << Padding << "Author: " << author << std::endl;
    }

    void SetTitle(StringView title) override
    {
      std::cout << Padding << "Title: " << title << std::endl;
    }

    void SetAnnotation(StringView annotation) override
    {
      std::cout << Padding << "Annotation: " << annotation << std::endl;
    }

    void SetProperty(StringView name, StringView value) override
    {
      std::cout << Padding << name << "=" << value << std::endl;
    }

    void StartTrack(uint_t idx) override
    {
      std::cout << " Track " << idx << std::endl;
      Padding.assign(2, ' ');
    }

    void SetData(Binary::Container::Ptr data) override
    {
      std::cout << Padding << "Data of size " << data->Size() << std::endl;
    }

  private:
    String Padding;
  };

  void List(CmdlineIterator& arg)
  {
    const auto file = *arg;
    const auto data = OpenFile(file);
    Printer printer;
    Formats::Chiptune::MultiTrackContainer::Parse(*data, printer);
  }

  class Extractor : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    Extractor() = default;

    void SetAuthor(StringView /*author*/) override {}
    void SetTitle(StringView /*title*/) override {}
    void SetAnnotation(StringView /*annotation*/) override {}
    void SetProperty(StringView name, StringView value) override
    {
      if (name == Module::ATTR_FILENAME)
      {
        LastDataName = value;
      }
    }

    void StartTrack(uint_t idx) override
    {
      Flush();
      LastTrackIdx = idx;
      LastDataIdx = 0;
    }

    void SetData(Binary::Container::Ptr data) override
    {
      Flush();
      LastData = data;
      ++LastDataIdx;
    }

    void Flush()
    {
      if (LastData)
      {
        const auto& filename = LastDataName.empty() ? Strings::Format("track{}_data{}", LastTrackIdx, LastDataIdx)
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
    uint_t LastTrackIdx = 0;
    uint_t LastDataIdx = 0;
  };

  void Extract(CmdlineIterator& arg)
  {
    const auto file = *arg;
    const auto data = OpenFile(file);
    Extractor extractor;
    Formats::Chiptune::MultiTrackContainer::Parse(*data, extractor);
    extractor.Flush();
  }

  using ModeFunc = void (*)(CmdlineIterator&);

  struct ModeEntry
  {
    const char* Name;
    ModeFunc Func;
    const char* Help;
  };

  // clang-format off
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
      "   --property <name> <value>\n"
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
  // clang-format on

  void ListModes()
  {
    std::cout << "Use <mode> without other params to get specific information\n"
                 "Supported modes:"
              << std::endl;
    for (const auto& mode : MODES)
    {
      std::cout << " " << mode.Name << std::endl;
    }
  }
}  // namespace

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
    const auto mode = *++arg;
    ++arg;
    for (const auto& availMode : MODES)
    {
      if (mode == availMode.Name)
      {
        if (arg)
        {
          (availMode.Func)(arg);
        }
        else
        {
          std::cout << availMode.Help;
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
