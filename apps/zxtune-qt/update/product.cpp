/*
Abstract:
  Product implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "product.h"
#include "apps/version/api.h"
#include "apps/zxtune-qt/ui/utils.h"

namespace
{
  unsigned ExtractIndex(const QString& prefix, const QString& val)
  {
    const QString num = val.right(val.size() - prefix.size());
    bool ok = false;
    const int res = num.toInt(&ok);
    return ok ? static_cast<unsigned>(res) : 0;
  }

  unsigned ExtractRevision(const QString& rev)
  {
    static const QLatin1String PREFIX_REV("rev");
    static const QLatin1String PREFIX_R("r");
    static const QLatin1String PREFIX_EMPTY("");

    if (rev.startsWith(PREFIX_REV))
    {
      return ExtractIndex(PREFIX_REV, rev);
    }
    else if (rev.startsWith(PREFIX_R))
    {
      return ExtractIndex(PREFIX_R, rev);
    }
    else
    {
      return ExtractIndex(PREFIX_EMPTY, rev);
    }
  }

  class OSTraits
  {
  public:
    explicit OSTraits(const QString& str)
      : Name(str.toLower())
    {
    }

    bool IsWindows() const
    {
      static const QLatin1String WINDOWS("windows");
      static const QLatin1String MINGW("mingw");
      return Name == WINDOWS
          || Name == MINGW
      ;
    }

    bool IsLinux() const
    {
      static const QLatin1String LINUX("linux");
      return Name == LINUX;
    }

    bool IsLinuxDistro() const
    {
      static const QLatin1String ARCHLINUX("archlinux");
      static const QLatin1String UBUNTU("ubuntu");
      static const QLatin1String REDHAT("redhat");
      return Name == ARCHLINUX
          || Name == UBUNTU
          || Name == REDHAT
      ;
    }
  private:
    const QString Name;
  };

  class ArchTraits
  {
  public:
    explicit ArchTraits(const QString& str)
      : Name(str.toLower())
    {
    }

    bool IsX86() const
    {
      static const QLatin1String X86("x86");
      static const QLatin1String I386("i386");
      static const QLatin1String I686("i686");
      return Name == X86
          || Name == I386
          || Name == I686
      ;
    }

    bool IsAmd64() const
    {
      static const QLatin1String X86_64("x86_64");
      static const QLatin1String AMD64("amd64");
      return Name == X86_64
          || Name == AMD64
      ;
    }
  private:
    const QString Name;
  };
}

namespace Product
{
  bool Version::IsNewerThan(const Version& rh) const
  {
    const unsigned revision = ExtractRevision(Index);
    const unsigned rhRevision = ExtractRevision(rh.Index);
    if (revision && rhRevision)
    {
      return revision > rhRevision;
    }
    if (ReleaseDate.isValid() && rh.ReleaseDate.isValid())
    {
      return ReleaseDate > rh.ReleaseDate;
    }
    return false;
  }

  bool Platform::IsReplaceableWith(const Platform& rh) const
  {
    if (*this == rh)
    {
      return true;
    }
    const OSTraits os(OperatingSystem);
    const OSTraits rhOs(rh.OperatingSystem);
    const ArchTraits arch(Architecture);
    const ArchTraits rhArch(rh.Architecture);
    if (os.IsWindows() && rhOs.IsWindows())
    {
      //on windows x86_64 can run any platform
      return arch.IsAmd64() || rhArch.IsX86();
    }
    else if (arch.IsX86() != rhArch.IsX86())
    {
      //arch incompatibility
      return false;
    }
    else if (os.IsLinuxDistro())
    {
      //distro can be replaced with common package
      return rhOs.IsLinux();
    }
    else
    {
      return false;
    }
  }

  Version CurrentBuildVersion()
  {
    const QDate date = QDate::fromString(ToQString(GetBuildDate()), Qt::SystemLocaleShortDate);
    return Version(ToQString(GetProgramVersion()), date);
  }

  Platform CurrentBuildPlatform()
  {
    return Platform(ToQString(GetBuildArchitecture()), ToQString(GetBuildPlatform()));
  }
}
