/**
*
* @file
*
* @brief  Portable Sound Format dumper utilities
*
* @author vitamin.caig@gmail.com
*
**/

#include "../../utils.h"
#include <make_ptr.h>
#include <formats/chiptune/emulation/portablesoundformat.h>
#include <formats/chiptune/emulation/playstationsoundformat.h>
#include <formats/chiptune/emulation/playstation2soundformat.h>
#include <strings/format.h>
#include <time/duration.h>
#include <memory>

namespace
{
  class SectionDumper
  {
  public:
    using Ptr = std::unique_ptr<const SectionDumper>;
    
    virtual void DumpReserved(const Binary::Container& blob) const = 0;
    virtual void DumpProgram(const Binary::Container& blob) const = 0;
  };
  
  class SimpleDumper : public SectionDumper
  {
  public:
    void DumpReserved(const Binary::Container& blob) const override
    {
      std::cout << " Reserved area: " << blob.Size() << " bytes" << std::endl;
    }
    
    void DumpProgram(const Binary::Container& blob) const override
    {
      std::cout << " Program area: " << blob.Size() << " bytes" << std::endl;
    }
  };
  
  class PSF1Dumper : public SimpleDumper
  {
  public:
    void DumpProgram(const Binary::Container& blob) const override
    {
      SimpleDumper::DumpProgram(blob);
      try
      {
        PSXExeDumper delegate;
        Formats::Chiptune::PlaystationSoundFormat::ParsePSXExe(blob, delegate);
      }
      catch (const std::exception&)
      {
        std::cout << "  Corrupted PS-X EXE" << std::endl;
      }
    }
    
  private:
    class PSXExeDumper : public Formats::Chiptune::PlaystationSoundFormat::Builder
    {
    public:
      void SetRegisters(uint32_t pc, uint32_t gp) override
      {
        std::cout << Strings::Format("  PC=0x%08x GP=0x%08x", pc, gp) << std::endl;
      }
      
      void SetStackRegion(uint32_t head, uint32_t size) override
      {
        std::cout << Strings::Format("  Stack %u bytes at 0x%08x", size, head) << std::endl;
      }
      
      void SetRegion(String region, uint_t fps) override
      {
        std::cout << "  Region: " << region;
        if (fps)
        {
           std::cout << " (" << fps << " fps)";
        }
        std::cout << std::endl;
      }

      void SetTextSection(uint32_t address, const Binary::Data& content)
      {
        std::cout << Strings::Format("  Text section: %1% (0x%1$08x) bytes at 0x%2$08x", content.Size(), address) << std::endl;
      };
    };
  };
  
  class PSF2Dumper : public SimpleDumper
  {
  public:
    void DumpReserved(const Binary::Container& blob) const override
    {
      SimpleDumper::DumpReserved(blob);
      try
      {
        PSF2VFSDumper delegate;
        Formats::Chiptune::Playstation2SoundFormat::ParseVFS(blob, delegate);
      }
      catch (const std::exception&)
      {
        std::cout << "  Corrupted PSF2 VFS" << std::endl;
      }
    }
  private:
    class PSF2VFSDumper : public Formats::Chiptune::Playstation2SoundFormat::Builder
    {
    public:
      void OnFile(String path, Binary::Container::Ptr content) override
      {
        std::cout << "  " << path << " (" << (content ? content->Size() : std::size_t(0)) << " bytes)" << std::endl;
      }
    };
  };

  class PSFDumper : public Formats::Chiptune::PortableSoundFormat::Builder
  {
  public:
    void SetVersion(uint_t ver) override
    {
      std::cout << " Type: " << DecodeVersion(ver) << std::endl;
      Dumper = CreateDumper(ver);
    }

    void SetReservedSection(Binary::Container::Ptr blob) override
    {
      Dumper->DumpReserved(*blob);
    }
    
    void SetProgramSection(Binary::Container::Ptr blob) override
    {
      Dumper->DumpProgram(*blob);
    }
    
    void SetTitle(String title) override
    {
      std::cout << " Title: " << title << std::endl;
    }
    
    virtual void SetArtist(String artist) override
    {
      std::cout << " Artist: " << artist << std::endl;
    }
    
    void SetGame(String game) override
    {
      std::cout << " Game: " << game << std::endl;
    }
    
    void SetYear(String date) override
    {
      std::cout << " Year: " << date << std::endl;
    }
    
    void SetGenre(String genre) override
    {
      std::cout << " Genre: " << genre << std::endl;
    }
    
    void SetComment(String comment) override
    {
      std::cout << " Comment: " << comment << std::endl;
    }
    
    void SetCopyright(String copyright) override
    {
      std::cout << " Copyright: " << copyright << std::endl;
    }
    
    void SetDumper(String dumper) override
    {
      std::cout << " Dumper: " << dumper << std::endl;
    }
    
    void SetLength(Time::Milliseconds duration) override
    {
      std::cout << " Length: " << Time::MillisecondsDuration(duration.Get(), Time::Milliseconds(1)).ToString() << std::endl;
    }
    
    void SetFade(Time::Milliseconds fade) override
    {
      std::cout << " Fade: " << Time::MillisecondsDuration(fade.Get(), Time::Milliseconds(1)).ToString() << std::endl;
    }
    
    void SetTag(String name, String value) override
    {
      std::cout << " " << name << ": " << value << std::endl;
    }

    void SetLibrary(uint_t num, String filename) override
    {
      std::cout << " Library #" << num << ": " << filename << std::endl;
    }
    
  private:
    static std::string DecodeVersion(uint_t ver)
    {
      switch (ver)
      {
      case 0x01:
        return "Playstation (PSF1)";
      case 0x02:
        return "Playstation 2 (PSF2)";
      case 0x11:
        return "Saturn (SSF)";
      case 0x12:
        return "Dreamcast (DSF)";
      case 0x13:
        return "Sega Genesis";
      case 0x21:
        return "Nintendo 64 (USF)";
      case 0x22:
        return "GameBoy Advance (GSF)";
      case 0x23: 
        return "Super NES (SNSF)";
      case 0x41:
        return "Capcom QSound (QSF)";
      default:
        return Strings::Format("Unknown type (%u)", ver);
      }
    }
    
    static SectionDumper::Ptr CreateDumper(uint_t ver)
    {
      switch (ver)
      {
      case 0x01:
        return MakePtr<PSF1Dumper>();
      case 0x02:
        return MakePtr<PSF2Dumper>();
      default:
        return MakePtr<SimpleDumper>();
      }
    }

  private:
    SectionDumper::Ptr Dumper;
  };
  
  /*
  Binary::Data::Ptr Reparse(const Binary::Container& data)
  {
    try
    {
      const auto builder = Formats::Chiptune::PortableSoundFormat::CreateBlobBuilder();
      Formats::Chiptune::PortableSoundFormat::Parse(data, *builder);
      return builder->GetResult();
    }
    catch (const std::exception&)
    {
      return Binary::Data::Ptr();
    }
  }
  */
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    for (int arg = 1; arg < argc; ++arg)
    {
      const std::string filename(argv[arg]);
      std::cout << filename << ':' << std::endl;
      std::unique_ptr<Dump> rawData(new Dump());
      Test::OpenFile(filename, *rawData);
      const Binary::Container::Ptr data = Binary::CreateContainer(std::move(rawData));
      PSFDumper builder;
      if (const auto container = Formats::Chiptune::PortableSoundFormat::Parse(*data, builder))
      {
        std::cout << "Done. Processed " << container->Size() << " bytes" << std::endl;
      }
      else
      {
        std::cout << "Failed to parse" << std::endl;
      }
      /*
      if (const auto recreated = Reparse(*data))
      {
        std::ofstream out(filename + ".zxtune", std::ios::binary);
        out.write(static_cast<const char*>(recreated->Start()), recreated->Size());
      }
      */
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
