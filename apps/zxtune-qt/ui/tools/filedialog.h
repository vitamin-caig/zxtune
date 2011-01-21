/*
Abstract:
  File dialog declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_FILEDIALOG_DEFINED
#define ZXTUNE_QT_FILEDIALOG_DEFINED

//boost includes
#include <boost/shared_ptr.hpp>
//qt includes
#include <QtCore/QStringList>

class QWidget;

class FileDialog
{
public:
  typedef boost::shared_ptr<FileDialog> Ptr;
  virtual ~FileDialog() {}

  virtual bool OpenSingleFile(const QString& title, const QString& filters, QString& file) = 0;
  virtual bool OpenMultipleFiles(const QString& title, const QString& filters, QStringList& files) = 0;
  virtual bool OpenFolder(const QString& title, QString& folder) = 0;
  virtual bool SaveFile(const QString& title, const QString& suffix, const QString& filter, QString& filename) = 0;

  static Ptr Create(QWidget& parent);
};

#endif //ZXTUNE_QT_FILEDIALOG_DEFINED
