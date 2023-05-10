#include <binary/data_builder.h>
#include <formats/packed/decoders.h>
#include <iostream>
#include <make_ptr.h>
#include <stdexcept>

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

namespace
{
  typedef Formats::Packed::Decoder::Ptr (*CreateDecoderFunc)();

  struct DecoderTraits
  {
    const StringView Id;
    const CreateDecoderFunc Create;
  };

  using namespace Formats::Packed;

  const DecoderTraits DECODERS[] = {
      // depackers
      {"HOBETA"_sv, &CreateHobetaDecoder},
      {"HRUST1"_sv, &CreateHrust1Decoder},
      {"HRUST2"_sv, &CreateHrust21Decoder},
      {"HRUST23"_sv, &CreateHrust23Decoder},
      {"FDI"_sv, &CreateFullDiskImageDecoder},
      {"DSQ"_sv, &CreateDataSquieezerDecoder},
      {"MSP"_sv, &CreateMSPackDecoder},
      {"TRUSH"_sv, &CreateTRUSHDecoder},
      {"LZS"_sv, &CreateLZSDecoder},
      {"PCD61"_sv, &CreatePowerfullCodeDecreaser61Decoder},
      {"PCD61i"_sv, &CreatePowerfullCodeDecreaser61iDecoder},
      {"PCD62"_sv, &CreatePowerfullCodeDecreaser62Decoder},
      {"HRUM"_sv, &CreateHrumDecoder},
      {"CC3"_sv, &CreateCodeCruncher3Decoder},
      {"CC4"_sv, &CreateCompressorCode4Decoder},
      {"CC4PLUS"_sv, &CreateCompressorCode4PlusDecoder},
      {"ESV"_sv, &CreateESVCruncherDecoder},
      {"GAM"_sv, &CreateGamePackerDecoder},
      {"GAMPLUS"_sv, &CreateGamePackerPlusDecoder},
      {"TLZ"_sv, &CreateTurboLZDecoder},
      {"TLZP"_sv, &CreateTurboLZProtectedDecoder},
      {"CHARPRES"_sv, &CreateCharPresDecoder},
      {"PACK2"_sv, &CreatePack2Decoder},
      {"LZH1"_sv, &CreateLZH1Decoder},
      {"LZH2"_sv, &CreateLZH2Decoder},
      {"SNA128"_sv, &CreateSna128Decoder},
      {"TD0"_sv, &CreateTeleDiskImageDecoder},
      {"Z80V145"_sv, &CreateZ80V145Decoder},
      {"Z80V20"_sv, &CreateZ80V20Decoder},
      {"Z80V30"_sv, &CreateZ80V30Decoder},
      {"MEGALZ"_sv, &CreateMegaLZDecoder},
      {"DSK"_sv, &CreateDSKDecoder},
      {"GZIP"_sv, &CreateGzipDecoder},
  };

  class AutoDecoder : public Formats::Packed::Decoder
  {
  public:
    virtual String GetDescription() const
    {
      return "Autodetect";
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      throw std::runtime_error("Should not be called");
    }

    virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      for (const auto& trait : DECODERS)
      {
        const auto delegate = (trait.Create)();
        if (auto result = delegate->Decode(rawData))
        {
          std::cerr << "Detected " << delegate->GetDescription() << std::endl;
          return result;
        }
      }
      return Formats::Packed::Container::Ptr();
    }
  };

  StringView GetType(int argc, const char* argv[])
  {
    switch (argc)
    {
    case 1:
      return {};
    case 2:
      return argv[1];
    default:
      return "--help"_sv;
    }
  }

  void ShowHelp()
  {
    std::cerr << "Usage:\n"
                 "  depacker [<type>]\n\n"
                 " to decode data from stdin to stdout\n\n";
  }

  void ListAvailableTypes()
  {
    std::cerr << "List of available types to disable detection:\n";
    for (const auto& trait : DECODERS)
    {
      std::cerr << trait.Id << std::endl;
    }
  }

  Formats::Packed::Decoder::Ptr CreateDecoder(StringView type)
  {
    if (type == ""_sv)
    {
      return MakePtr<AutoDecoder>();
    }
    else if (type == "--help"_sv)
    {
      ShowHelp();
    }
    else
    {
      for (const auto& decoder : DECODERS)
      {
        if (type == decoder.Id)
        {
          return (decoder.Create)();
        }
      }
      std::cerr << "Unknown type " << type << std::endl;
    }
    ListAvailableTypes();
    throw std::runtime_error("");
  }

  Binary::Container::Ptr ReadInput()
  {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), O_BINARY) == -1)
    {
      throw std::runtime_error("Failed to set binary mode to stdin");
    }
#endif
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

  void WriteOutput(Binary::View data)
  {
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
    {
      throw std::runtime_error("Failed to set binary mode to stdout");
    }
#endif
    std::cout.write(static_cast<const char*>(data.Start()), data.Size());
  }
}  // namespace

int main(int argc, const char* argv[])
{
  try
  {
    const auto type = GetType(argc, argv);
    const auto decoder = CreateDecoder(type);
    const auto input = ReadInput();
    std::cerr << "Input: " << input->Size() << std::endl;
    if (const auto output = decoder->Decode(*input))
    {
      std::cerr << "Payload: " << output->PackedSize() << std::endl;
      WriteOutput(*output);
      std::cerr << "Output: " << output->Size() << std::endl;
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
