/**
 *
 * @file
 *
 * @brief File dialog factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QStringList>

namespace UI
{
  bool OpenSingleFileDialog(const QString& title, const QString& filters, QString& file);
  bool OpenMultipleFilesDialog(const QString& title, const QString& filters, QStringList& files);
  bool OpenFolderDialog(const QString& title, QString& folder);
  bool SaveFileDialog(const QString& title, const QString& suffix, const QStringList& filters, QString& filename,
                      int* usedFilter = nullptr);
};  // namespace UI
