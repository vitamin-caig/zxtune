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

//qt includes
#include <QtCore/QStringList>

namespace UI
{
  bool OpenSingleFileDialog(const QString& title, const QString& filters, QString& file);
  bool OpenMultipleFilesDialog(const QString& title, const QString& filters, QStringList& files);
  bool OpenFolderDialog(const QString& title, QString& folder);
  bool SaveFileDialog(const QString& title, const QString& suffix, const QStringList& filters, QString& filename, int* usedFilter = 0);
};

#endif //ZXTUNE_QT_FILEDIALOG_DEFINED
