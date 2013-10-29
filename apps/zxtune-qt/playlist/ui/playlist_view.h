/**
* 
* @file
*
* @brief Playlist view interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "playlist/supp/controller.h"
//qt includes
#include <QtGui/QWidget>

namespace Playlist
{
  namespace Item
  {
    class Data;
  }

  namespace UI
  {
    class View : public QWidget
    {
      Q_OBJECT
    protected:
      explicit View(QWidget& parent);
    public:
      static View* Create(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params);

      virtual Playlist::Controller::Ptr GetPlaylist() const = 0;
    public slots:
      virtual void AddItems(const QStringList& items) = 0;

      //navigate
      virtual void Play() = 0;
      virtual void Pause() = 0;
      virtual void Stop() = 0;
      virtual void Finish() = 0;
      virtual void Next() = 0;
      virtual void Prev() = 0;
      virtual void Clear() = 0;
      virtual void AddFiles() = 0;
      virtual void AddFolder() = 0;
      //actions
      virtual void Save() = 0;
      virtual void Rename() = 0;
    private slots:
      virtual void ActivateItem(unsigned idx, Playlist::Item::Data::Ptr data) = 0;
      virtual void LongOperationStart() = 0;
      virtual void LongOperationStop() = 0;
      virtual void LongOperationCancel() = 0;
    signals:
      void Renamed(const QString&);
      void ItemActivated(Playlist::Item::Data::Ptr);
    };
  }
}
