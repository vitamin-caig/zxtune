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
// std includes
#include <cctype>
// library includes
#include <formats/chiptune/emulation/playstation2soundformat.h>

namespace Module
{
  namespace PSF
  {
    String PsxVfs::ToUpper(const char* str)
    {
      String res;
      res.reserve(256);
      while (*str)
      {
        res.push_back(std::toupper(*str));
        ++str;
      }
      return res;
    }

    class VfsParser : public Formats::Chiptune::Playstation2SoundFormat::Builder
    {
    public:
      explicit VfsParser(PsxVfs& vfs)
        : Vfs(vfs)
      {}

      void OnFile(String path, Binary::Container::Ptr content) override
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
  }  // namespace PSF
}  // namespace Module
