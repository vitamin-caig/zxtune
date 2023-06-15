/**
 *
 * @file
 *
 * @brief Search dialog interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <playlist/supp/operations.h>

class QWidget;

namespace Playlist::UI
{
  Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent);
}  // namespace Playlist::UI
