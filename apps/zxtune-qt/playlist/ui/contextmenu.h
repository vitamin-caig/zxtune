/**
* 
* @file
*
* @brief Playlist context menu interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

class QPoint;

namespace Playlist
{
  class Controller;
  namespace UI
  {
    class TableView;
    void ExecuteContextMenu(const QPoint& pos, TableView& view, Controller& playlist);
  }
}
