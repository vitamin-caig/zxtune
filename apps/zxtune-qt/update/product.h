/*
Abstract:
  Product interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_PRODUCT_H_DEFINED
#define ZXTUNE_QT_UPDATE_PRODUCT_H_DEFINED

//qt includes
#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace Product
{
  struct Version
  {
    QString Index;
    QDate ReleaseDate;

    Version(const QString& idx, const QDate& date)
      : Index(idx)
      , ReleaseDate(date)
    {
    }

    //! @return true if this refers to more recent version
    bool IsNewerThan(const Version& rh) const;

    bool operator == (const Version& rh) const
    {
      return Index == rh.Index && ReleaseDate == rh.ReleaseDate;
    }
  };

  struct Platform
  {
    QString Architecture;
    QString OperatingSystem;

    Platform(const QString& arch, const QString& os)
      : Architecture(arch)
      , OperatingSystem(os)
    {
    }

    //! @return true if this can be replaced with rh
    //! @example mingw and windows are fully compatible for the same versions
    //! @example x86_64 can be replaced with x86 for windows
    //! @example Debian amd64 can be replaced with Linux x86_64
    bool IsReplaceableWith(const Platform& rh) const;

    bool operator == (const Platform& rh) const
    {
      return Architecture == rh.Architecture
          && OperatingSystem == rh.OperatingSystem
      ;
    }
  };

  struct Download
  {
    QUrl Description;
    QUrl Package;

    Download(const QUrl& desc, const QUrl& pkg)
      : Description(desc)
      , Package(pkg)
    {
    }
  };

  //Current build attrs
  Version CurrentBuildVersion();
  Platform CurrentBuildPlatform();
}

#endif //ZXTUNE_QT_UPDATE_PRODUCT_H_DEFINED
