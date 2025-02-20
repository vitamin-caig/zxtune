/**
 *
 * @file
 *
 * @brief Product entity interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include <memory>
#include <vector>

namespace Product
{
  class Release
  {
  public:
    virtual ~Release() = default;

    enum PlatformTag
    {
      UNKNOWN_PLATFORM,
      WINDOWS,
      MINGW,
      LINUX,
      DINGUX,
      MACOSX,
    };

    enum ArchitectureTag
    {
      UNKNOWN_ARCHITECTURE,
      X86,
      X86_64,
      ARM,
      ARM64,
      ARMHF,
      LOONG64,
      MIPSEL,
    };

    virtual PlatformTag Platform() const = 0;
    virtual ArchitectureTag Architecture() const = 0;
    virtual QString Version() const = 0;
    virtual QDate Date() const = 0;
  };

  class Update : public Release
  {
  public:
    using Ptr = std::shared_ptr<const Update>;

    enum PackagingTag
    {
      UNKNOWN_PACKAGING,
      ZIP,
      TARGZ,
      TARXZ,
      DEB,
      RPM,
      DMG,
    };

    enum TypeTag
    {
      UNKNOWN_TYPE,
      WINDOWS_X86,
      WINDOWS_X86_64,
      MINGW_X86,
      MINGW_X86_64,
      MINGW_ARM64,
      LINUX_X86,
      LINUX_X86_64,
      LINUX_ARM,
      LINUX_ARM64,
      LINUX_ARMHF,
      LINUX_LOONG64,
      DINGUX_MIPSEL,
      ARCHLINUX_X86,
      ARCHLINUX_X86_64,
      UBUNTU_X86,
      UBUNTU_X86_64,
      REDHAT_X86,
      REDHAT_X86_64,
      MACOSX_X86_64,
      MACOSX_ARM64,
    };

    virtual PackagingTag Packaging() const = 0;
    virtual QString Title() const = 0;
    virtual QUrl Description() const = 0;
    virtual QUrl Package() const = 0;
  };

  const Release& ThisRelease();
  Update::TypeTag GetUpdateType(Release::PlatformTag platform, Release::ArchitectureTag architecture,
                                Update::PackagingTag packaging);
  std::vector<Update::TypeTag> SupportedUpdateTypes();
}  // namespace Product
