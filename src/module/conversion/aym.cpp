/**
 *
 * @file
 *
 * @brief  AYM-based conversion implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/aym/dumper.h"
#include "module/conversion/api.h"
#include "module/conversion/parameters.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_chiptune.h"

#include "binary/data.h"
#include "core/core_parameters.h"
#include "l10n/api.h"
#include "module/attributes.h"
#include "parameters/accessor.h"
#include "parameters/identifier.h"
#include "time/duration.h"

#include "error.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include <memory>
#include <utility>

namespace Module
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("module_conversion");

  class BaseDumperParameters : public Devices::AYM::DumperParameters
  {
  public:
    explicit BaseDumperParameters(uint_t opt)
      : Optimization(static_cast<Devices::AYM::DumperParameters::Optimization>(opt))
    {}

    Time::Microseconds FrameDuration() const override
    {
      return AYM::BASE_FRAME_DURATION;
    }

    Devices::AYM::DumperParameters::Optimization OptimizationLevel() const override
    {
      return Optimization;
    }

  private:
    const Devices::AYM::DumperParameters::Optimization Optimization;
  };

  class FYMDumperParameters : public Devices::AYM::FYMDumperParameters
  {
  public:
    FYMDumperParameters(Parameters::Accessor::Ptr params, uint_t loopFrame, uint_t opt)
      : Base(opt)
      , Params(std::move(params))
      , Loop(loopFrame)
    {}

    Time::Microseconds FrameDuration() const override
    {
      return Base.FrameDuration();
    }

    Devices::AYM::DumperParameters::Optimization OptimizationLevel() const override
    {
      return Base.OptimizationLevel();
    }

    uint64_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      return Parameters::GetInteger<uint64_t>(*Params, CLOCKRATE, CLOCKRATE_DEFAULT);
    }

    String Title() const override
    {
      return Parameters::GetString(*Params, Module::ATTR_TITLE);
    }

    String Author() const override
    {
      return Parameters::GetString(*Params, Module::ATTR_AUTHOR);
    }

    uint_t LoopFrame() const override
    {
      return Loop;
    }

  private:
    const BaseDumperParameters Base;
    const Parameters::Accessor::Ptr Params;
    const uint_t Loop;
  };

  // aym-based conversion
  Binary::Data::Ptr ConvertAYMFormat(const AYM::Holder& holder, const Conversion::Parameter& spec,
                                     const Parameters::Accessor::Ptr& params)
  {
    using namespace Conversion;

    Devices::AYM::Dumper::Ptr dumper;
    String errMessage;

    // convert to PSG
    if (const auto* psg = parameter_cast<PSGConvertParam>(&spec))
    {
      const BaseDumperParameters dumpParams(psg->Optimization);
      dumper = Devices::AYM::CreatePSGDumper(dumpParams);
      errMessage = translate("Failed to convert to PSG format.");
    }
    // convert to ZX50
    else if (const auto* zx50 = parameter_cast<ZX50ConvertParam>(&spec))
    {
      const BaseDumperParameters dumpParams(zx50->Optimization);
      dumper = Devices::AYM::CreateZX50Dumper(dumpParams);
      errMessage = translate("Failed to convert to ZX50 format.");
    }
    // convert to debugay
    else if (const auto* dbg = parameter_cast<DebugAYConvertParam>(&spec))
    {
      const BaseDumperParameters dumpParams(dbg->Optimization);
      dumper = Devices::AYM::CreateDebugDumper(dumpParams);
      errMessage = translate("Failed to convert to debug ay format.");
    }
    // convert to aydump
    else if (const auto* aydump = parameter_cast<AYDumpConvertParam>(&spec))
    {
      const BaseDumperParameters dumpParams(aydump->Optimization);
      dumper = Devices::AYM::CreateRawStreamDumper(dumpParams);
      errMessage = translate("Failed to convert to raw ay dump.");
    }
    // convert to fym
    else if (const auto* fym = parameter_cast<FYMConvertParam>(&spec))
    {
      auto dumpParams = MakePtr<FYMDumperParameters>(params, 0 /*LoopFrame - TODO*/, fym->Optimization);
      dumper = Devices::AYM::CreateFYMDumper(std::move(dumpParams));
      errMessage = translate("Failed to convert to FYM format.");
    }

    if (!dumper)
    {
      return {};
    }

    try
    {
      holder.Dump(*dumper);
      return dumper->GetDump();
    }
    catch (const Error& err)
    {
      throw Error(THIS_LINE, errMessage).AddSuberror(err);
    }
  }

  Binary::Data::Ptr Convert(const Holder& holder, const Conversion::Parameter& spec,
                            const Parameters::Accessor::Ptr& params)
  {
    using namespace Conversion;
    if (const auto* aymHolder = dynamic_cast<const AYM::Holder*>(&holder))
    {
      return ConvertAYMFormat(*aymHolder, spec, params);
    }
    return {};
  }
}  // namespace Module
