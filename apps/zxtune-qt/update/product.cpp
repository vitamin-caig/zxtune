/**
 *
 * @file
 *
 * @brief Product entity implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/update/product.h"

#include "apps/zxtune-qt/ui/utils.h"

#include "debug/log.h"
#include "platform/version/api.h"

#include <QtCore/QFileInfo>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");

  class CurrentRelease : public Product::Release
  {
  public:
    CurrentRelease() = default;

    Product::Release::PlatformTag Platform() const override
    {
      const String txt = Platform::Version::GetBuildPlatform();
      if (txt == "windows")
      {
        return Product::Release::WINDOWS;
      }
      else if (txt == "mingw")
      {
        return Product::Release::MINGW;
      }
      else if (txt == "linux")
      {
        return Product::Release::LINUX;
      }
      else if (txt == "dingux")
      {
        return Product::Release::DINGUX;
      }
      else if (txt == "darwin")
      {
        return Product::Release::MACOSX;
      }
      else
      {
        Dbg("Unknown platform {}", txt);
        return Product::Release::UNKNOWN_PLATFORM;
      }
    }

    Product::Release::ArchitectureTag Architecture() const override
    {
      const String txt = Platform::Version::GetBuildArchitecture();
      if (txt == "x86")
      {
        return Product::Release::X86;
      }
      else if (txt == "x86_64")
      {
        return Product::Release::X86_64;
      }
      else if (txt == "arm")
      {
        return Product::Release::ARM;
      }
      else if (txt == "aarch64")
      {
        return Product::Release::ARM64;
      }
      else if (txt == "armhf")
      {
        return Product::Release::ARMHF;
      }
      else if (txt == "loongarch64")
      {
        return Product::Release::LOONG64;
      }
      else if (txt == "mipsel")
      {
        return Product::Release::MIPSEL;
      }
      else
      {
        Dbg("Unknown architecture {}", txt);
        return Product::Release::UNKNOWN_ARCHITECTURE;
      }
    }

    QString Version() const override
    {
      return ToQString(Platform::Version::GetProgramVersion());
    }

    QDate Date() const override
    {
      return QDate::fromString(ToQString(Platform::Version::GetBuildDate()), "MMM d yyyy");
    }
  };

  using namespace Product;

  struct PackagingTraits
  {
    const char* ReleaseFile;
    Update::PackagingTag Packaging;
  };

  const PackagingTraits PACKAGING_TYPES[] = {
      {"arch-release", Update::TARXZ}, {"debian_version", Update::DEB}, {"debian_release", Update::DEB},
      {"redhat-release", Update::RPM}, {"redhat_version", Update::RPM}, {"fedora-release", Update::RPM},
      {"centos-release", Update::RPM}, {"SuSE-release", Update::RPM},
  };

  Update::PackagingTag GetLinuxPackaging()
  {
    static const QLatin1String RELEASE_DIR("/etc/");
    for (const auto& type : PACKAGING_TYPES)
    {
      if (QFileInfo(RELEASE_DIR + type.ReleaseFile).exists())
      {
        return type.Packaging;
      }
    }
    return Update::TARGZ;
  }

  struct ReleaseTypeTraits
  {
    Update::TypeTag Type;
    Release::PlatformTag Platform;
    Release::ArchitectureTag Architecture;
    Update::PackagingTag Packaging;
  };

  const ReleaseTypeTraits RELEASE_TYPES[] = {
      {Update::WINDOWS_X86, Release::WINDOWS, Release::X86, Update::ZIP},
      {Update::WINDOWS_X86_64, Release::WINDOWS, Release::X86_64, Update::ZIP},
      {Update::MINGW_X86, Release::MINGW, Release::X86, Update::ZIP},
      {Update::MINGW_X86_64, Release::MINGW, Release::X86_64, Update::ZIP},
      {Update::MINGW_ARM64, Release::MINGW, Release::ARM64, Update::ZIP},
      {Update::LINUX_X86, Release::LINUX, Release::X86, Update::TARGZ},
      {Update::LINUX_X86_64, Release::LINUX, Release::X86_64, Update::TARGZ},
      {Update::LINUX_ARM, Release::LINUX, Release::ARM, Update::TARGZ},
      {Update::LINUX_ARM64, Release::LINUX, Release::ARM64, Update::TARGZ},
      {Update::LINUX_ARMHF, Release::LINUX, Release::ARMHF, Update::TARGZ},
      {Update::LINUX_LOONG64, Release::LINUX, Release::LOONG64, Update::TARGZ},
      {Update::DINGUX_MIPSEL, Release::DINGUX, Release::MIPSEL, Update::TARGZ},
      {Update::ARCHLINUX_X86, Release::LINUX, Release::X86, Update::TARXZ},
      {Update::ARCHLINUX_X86_64, Release::LINUX, Release::X86_64, Update::TARXZ},
      {Update::UBUNTU_X86, Release::LINUX, Release::X86, Update::DEB},
      {Update::UBUNTU_X86_64, Release::LINUX, Release::X86_64, Update::DEB},
      {Update::REDHAT_X86, Release::LINUX, Release::X86, Update::RPM},
      {Update::REDHAT_X86_64, Release::LINUX, Release::X86_64, Update::RPM},
      {Update::MACOSX_X86_64, Release::MACOSX, Release::X86_64, Update::DMG},
      {Update::MACOSX_ARM64, Release::MACOSX, Release::ARM64, Update::DMG},
  };
}  // namespace

namespace Product
{
  const Release& ThisRelease()
  {
    static const CurrentRelease CURRENT;
    return CURRENT;
  }

  Update::TypeTag GetUpdateType(Release::PlatformTag platform, Release::ArchitectureTag architecture,
                                Update::PackagingTag packaging)
  {
    for (const auto& release : RELEASE_TYPES)
    {
      if (release.Platform == platform && release.Architecture == architecture && release.Packaging == packaging)
      {
        Dbg(" platform={} architecture={} packaging={} => {}", int(platform), int(architecture), int(packaging),
            int(release.Type));
        return release.Type;
      }
    }
    Dbg(" platform={} architecture={} packaging={} => unknown", int(platform), int(architecture), int(packaging));
    return Update::UNKNOWN_TYPE;
  }

  std::vector<Update::TypeTag> SupportedUpdateTypes()
  {
    Dbg("Supported update types:");
    std::vector<Update::TypeTag> result;
    const Release::PlatformTag platform = ThisRelease().Platform();
    const Release::ArchitectureTag architecture = ThisRelease().Architecture();

    // do not use cross-architecture update types
    switch (platform)
    {
    case Release::MINGW:
      result.push_back(GetUpdateType(Release::MINGW, architecture, Update::ZIP));
      result.push_back(GetUpdateType(Release::WINDOWS, architecture, Update::ZIP));
      break;
    case Release::WINDOWS:
      result.push_back(GetUpdateType(Release::WINDOWS, architecture, Update::ZIP));
      result.push_back(GetUpdateType(Release::MINGW, architecture, Update::ZIP));
      break;
    case Release::LINUX:
    {
      const Update::PackagingTag packaging = GetLinuxPackaging();
      const auto bestType = GetUpdateType(Release::LINUX, architecture, packaging);
      if (bestType != Update::UNKNOWN_TYPE)
      {
        result.push_back(bestType);
      }
      if (packaging != Update::TARGZ)
      {
        result.push_back(GetUpdateType(Release::LINUX, architecture, Update::TARGZ));
      }
    };
    break;
    case Release::DINGUX:
      result.push_back(GetUpdateType(platform, architecture, Update::TARGZ));
      break;
    case Release::MACOSX:
      result.push_back(GetUpdateType(platform, architecture, Update::DMG));
      break;
    default:
      break;
    };
    return result;
  }
}  // namespace Product
