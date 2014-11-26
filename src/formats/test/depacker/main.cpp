#include <binary/data_builder.h>
#include <formats/packed/decoders.h>
#include <boost/range/end.hpp>
#include <stdexcept>
#include <iostream>

#include <fcntl.h>
#include <io.h>

namespace
{
  typedef Formats::Packed::Decoder::Ptr (*CreateDecoderFunc)();

  struct DecoderTraits
  {
    const char* const Id;
    const CreateDecoderFunc Create;
  };
  
  using namespace Formats::Packed;

  const DecoderTraits DECODERS[] =
  {
    //depackers
    {"HOBETA",   &CreateHobetaDecoder                   },
    {"HRUST1",   &CreateHrust1Decoder                   },
    {"HRUST2",   &CreateHrust21Decoder                  },
    {"HRUST23",  &CreateHrust23Decoder                  },
    {"FDI",      &CreateFullDiskImageDecoder            },
    {"DSQ",      &CreateDataSquieezerDecoder            },
    {"MSP",      &CreateMSPackDecoder                   },
    {"TRUSH",    &CreateTRUSHDecoder                    },
    {"LZS",      &CreateLZSDecoder                      },
    {"PCD61",    &CreatePowerfullCodeDecreaser61Decoder },
    {"PCD61i",   &CreatePowerfullCodeDecreaser61iDecoder},
    {"PCD62",    &CreatePowerfullCodeDecreaser62Decoder },
    {"HRUM",     &CreateHrumDecoder                     },
    {"CC3",      &CreateCodeCruncher3Decoder            },
    {"CC4",      &CreateCompressorCode4Decoder          },
    {"CC4PLUS",  &CreateCompressorCode4PlusDecoder      },
    {"ESV",      &CreateESVCruncherDecoder              },
    {"GAM",      &CreateGamePackerDecoder               },
    {"GAMPLUS",  &CreateGamePackerPlusDecoder           },
    {"TLZ",      &CreateTurboLZDecoder                  },
    {"TLZP",     &CreateTurboLZProtectedDecoder         },
    {"CHARPRES", &CreateCharPresDecoder                 },
    {"PACK2",    &CreatePack2Decoder                    },
    {"LZH1",     &CreateLZH1Decoder                     },
    {"LZH2",     &CreateLZH2Decoder                     },
    {"SNA128",   &CreateSna128Decoder                   },
    {"TD0",      &CreateTeleDiskImageDecoder            },
    {"Z80V145",  &CreateZ80V145Decoder                  },
    {"Z80V20",   &CreateZ80V20Decoder                   },
    {"Z80V30",   &CreateZ80V30Decoder                   },
    {"MEGALZ",   &CreateMegaLZDecoder                   },
  };
  
  std::string GetType(int argc, const char* argv[])
  {
    if (argc < 2)
    {
      throw std::runtime_error("Decoder type should be specified (use --help)");
    }
    return std::string(argv[1]);
  }
  
  void ShowHelp()
  {
    std::cerr << "Usage:\n"
                 "  depacker <type>\n\n"
                 " to decode data from stdin to stdout\n\n";
  }
  
  void ListAvailableTypes()
  {
    std::cerr << "List of available types:\n";
    for (const DecoderTraits* decoder = DECODERS; decoder != boost::end(DECODERS); ++decoder)
    {
      std::cerr << decoder->Id << std::endl;
    }
  }
  
  Formats::Packed::Decoder::Ptr CreateDecoder(const std::string& type)
  {
    for (const DecoderTraits* decoder = DECODERS; decoder != boost::end(DECODERS); ++decoder)
    {
      if (type == decoder->Id)
      {
        return (decoder->Create)();
      }
    }
    if (type == "--help")
    {
      ShowHelp();
    }
    else
    {
      std::cerr << "Unknown type " << type << std::endl;
    }
    ListAvailableTypes();
    throw std::runtime_error("");
  }
  
  Binary::Container::Ptr ReadInput()
  {
    if (_setmode(_fileno(stdin), O_BINARY) == -1)
    {
      throw std::runtime_error("Failed to set binary mode to stdin");
    }
    Binary::DataBuilder builder(1024);
    std::size_t total = 0;
    for (;;)
    {
      const std::size_t PORTION = 1024;
      std::cin.read(static_cast<char*>(builder.Allocate(PORTION)), PORTION);
      const std::size_t done = std::cin.gcount();
      total += done;
      if (!done)
      {
        break;
      }
    }
    builder.Resize(total);
    return builder.CaptureResult();
  }
  
  void WriteOutput(const Binary::Data& data)
  {
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
    {
      throw std::runtime_error("Failed to set binary mode to stdout");
    }
    std::cout.write(static_cast<const char*>(data.Start()), data.Size());
  }
}

int main(int argc, const char* argv[])
{
  try
  {
    const std::string type = GetType(argc, argv);
    const Formats::Packed::Decoder::Ptr decoder = CreateDecoder(type);
    const Binary::Container::Ptr input = ReadInput();
    std::cerr << "Read " << input->Size() << " bytes\n";
    if (const Binary::Container::Ptr output = decoder->Decode(*input))
    {
      WriteOutput(*output);
    }
    else
    {
      throw std::runtime_error("Wrong format");
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
