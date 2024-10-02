/**
 *
 * @file
 *
 * @brief  PSF related stuff implementation. Vfs
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/psf_vfs.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <formats/chiptune/emulation/playstation2soundformat.h>
#include <strings/casing.h>

namespace Module::PSF
{
  String PsxVfs::Normalize(StringView str)
  {
    return Strings::ToUpperAscii(str);
  }

  class VfsParser : public Formats::Chiptune::Playstation2SoundFormat::Builder
  {
  public:
    explicit VfsParser(PsxVfs& vfs)
      : Vfs(vfs)
    {}

    void OnFile(StringView path, Binary::Container::Ptr content) override
    {
      Vfs.Add(path, std::move(content));
    }

  private:
    PsxVfs& Vfs;
  };

  void PsxVfs::Parse(const Binary::Container& data, PsxVfs& vfs)
  {
    VfsParser parser(vfs);
    Formats::Chiptune::Playstation2SoundFormat::ParseVFS(data, parser);
  }
}  // namespace Module::PSF
