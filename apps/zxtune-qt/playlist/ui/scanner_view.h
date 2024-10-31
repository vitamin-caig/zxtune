/**
 *
 * @file
 *
 * @brief Scanner view interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/scanner.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

namespace Playlist::UI
{
  class ScannerView : public QWidget
  {
    Q_OBJECT
  protected:
    explicit ScannerView(QWidget& parent);

  public:
    static ScannerView* Create(QWidget& parent, Playlist::Scanner::Ptr scanner);
  };
}  // namespace Playlist::UI
