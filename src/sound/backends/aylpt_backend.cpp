/**
 *
 * @file
 *
 * @brief  AYLPT backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/conversion/api.h"
#include "module/conversion/parameters.h"
#include "sound/backends/aylpt.h"
#include "sound/backends/backend_impl.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"

#include "binary/data.h"
#include "debug/log.h"
#include "devices/aym.h"
#include "l10n/api.h"
#include "module/holder.h"
#include "module/state.h"
#include "parameters/accessor.h"
#include "platform/shared_library.h"
#include "sound/backend.h"
#include "sound/backend_attrs.h"
#include "sound/chunk.h"
#include "time/instant.h"

#include "error.h"
#include "make_ptr.h"
#include "string_view.h"
#include "types.h"

#include <chrono>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace Sound::AyLpt
{
  const Debug::Stream Dbg("Sound::Backend::Aylpt");

  const uint_t CAPABILITIES = CAP_TYPE_HARDWARE;

  // http://logix4u.net/component/content/article/14-parallel-port/15-a-tutorial-on-parallel-port-interfacing
  // http://bulba.untergrund.net/LPT-YM.7z
  enum
  {
    DATA_PORT = 0x378,
    CONTROL_PORT = DATA_PORT + 2,

    // pin14 (Control-1) -> ~BDIR
    PIN_NOWRITE = 0x2,
    // pin16 (Control-2) -> BC1
    PIN_ADDR = 0x4,
    // pin17 (Control-3) -> ~RESET
    PIN_NORESET = 0x8,
    // other unused pins
    PIN_UNUSED = 0xf1,

    CMD_SELECT_ADDR = PIN_ADDR | PIN_NORESET | PIN_UNUSED,
    CMD_SELECT_DATA = PIN_NORESET | PIN_UNUSED,
    CMD_WRITE_COMMIT = PIN_NOWRITE | PIN_NORESET | PIN_UNUSED,
    CMD_RESET_START = PIN_ADDR | PIN_UNUSED,
    CMD_RESET_STOP = PIN_NORESET | PIN_ADDR | PIN_UNUSED,
  };

  static_assert(CMD_SELECT_ADDR == 0xfd, "Invariant");
  static_assert(CMD_SELECT_DATA == 0xf9, "Invariant");
  static_assert(CMD_WRITE_COMMIT == 0xfb, "Invariant");
  static_assert(CMD_RESET_START == 0xf5, "Invariant");
  static_assert(CMD_RESET_STOP == 0xfd, "Invariant");

  class LptPort
  {
  public:
    using Ptr = std::shared_ptr<LptPort>;
    virtual ~LptPort() = default;

    virtual void Control(uint_t val) = 0;
    virtual void Data(uint_t val) = 0;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Binary::Data::Ptr data, LptPort::Ptr port)
      : Data(std::move(data))
      , Port(std::move(port))
    {}

    ~BackendWorker() override = default;

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return VolumeControl::Ptr();
    }

    void Startup() override
    {
      Reset();
      Dbg("Successfull start");
      NextFrameTime = std::chrono::steady_clock::now();
      FrameDuration = std::chrono::microseconds(20000);  // TODO
    }

    void Shutdown() override
    {
      Reset();
      Dbg("Successfull shut down");
    }

    void Pause() override
    {
      Reset();
    }

    void Resume() override {}

    void FrameStart(const Module::State& state) override
    {
      WaitForNextFrame();
      const auto frame = state.At().CastTo<Time::Microsecond>().Get() / FrameDuration.count();
      const uint8_t* regs = static_cast<const uint8_t*>(Data->Start()) + frame * Devices::AYM::Registers::TOTAL;
      WriteRegisters(regs);
    }

    void FrameFinish(Chunk /*buffer*/) override
    {
      NextFrameTime += FrameDuration;
    }

  private:
    void Reset()
    {
      Port->Control(CMD_RESET_STOP);
      Port->Control(CMD_RESET_START);
      Delay();
      Port->Control(CMD_RESET_STOP);
    }

    void Write(uint_t reg, uint_t val)
    {
      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(reg);
      Port->Control(CMD_SELECT_ADDR);
      Port->Data(reg);

      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(val);
      Port->Control(CMD_SELECT_DATA);
      Port->Data(val);
      Port->Control(CMD_WRITE_COMMIT);
    }

    void WaitForNextFrame()
    {
      std::this_thread::sleep_until(NextFrameTime);
    }

    static void Delay()
    {
      // according to datasheets, maximal timing is reset pulse width 5uS
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    void WriteRegisters(const uint8_t* data)
    {
      for (uint_t idx = 0; idx <= Devices::AYM::Registers::ENV; ++idx)
      {
        const uint_t reg = Devices::AYM::Registers::ENV - idx;
        const uint_t val = data[reg];
        if (reg != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Write(reg, val);
        }
      }
    }

  private:
    const Binary::Data::Ptr Data;
    const LptPort::Ptr Port;
    std::chrono::steady_clock::time_point NextFrameTime;
    std::chrono::microseconds FrameDuration;
  };

  class DlPortIO : public LptPort
  {
  public:
    explicit DlPortIO(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
      , WriteByte(reinterpret_cast<WriteFunctionType>(Lib->GetSymbol("DlPortWritePortUchar")))
    {}

    void Control(uint_t val) override
    {
      WriteByte(CONTROL_PORT, val);
    }

    void Data(uint_t val) override
    {
      WriteByte(DATA_PORT, val);
    }

  private:
    const Platform::SharedLibrary::Ptr Lib;
    typedef void(__stdcall* WriteFunctionType)(unsigned short port, unsigned char val);
    const WriteFunctionType WriteByte;
  };

  class DllName : public Platform::SharedLibrary::Name
  {
  public:
    StringView Base() const override
    {
      return "dlportio"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"inpout32.dll"sv, "inpoutx64.dll"sv};
    }
  };

  LptPort::Ptr LoadLptLibrary()
  {
    static const DllName NAME;
    auto lib = Platform::SharedLibrary::Load(NAME);
    return MakePtr<DlPortIO>(std::move(lib));
  }

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(LptPort::Ptr port)
      : Port(std::move(port))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const override
    {
      static const Module::Conversion::AYDumpConvertParam spec;
      if (const Binary::Data::Ptr data = Module::Convert(*holder, spec, params))
      {
        return MakePtr<BackendWorker>(data, Port);
      }
      throw Error(THIS_LINE, translate("Real AY via LPT is not supported for this module."));
    }

  private:
    const LptPort::Ptr Port;
  };
}  // namespace Sound::AyLpt

namespace Sound
{
  void RegisterAyLptBackend(BackendsStorage& storage)
  {
    try
    {
      auto port = AyLpt::LoadLptLibrary();
      auto factory = MakePtr<AyLpt::BackendWorkerFactory>(std::move(port));
      storage.Register(AyLpt::BACKEND_ID, AyLpt::BACKEND_DESCRIPTION, AyLpt::CAPABILITIES, std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(AyLpt::BACKEND_ID, AyLpt::BACKEND_DESCRIPTION, AyLpt::CAPABILITIES, e);
    }
  }
}  // namespace Sound
