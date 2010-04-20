/*
Abstract:
  Seek controller creation implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "../seekcontrol.h"
#include "seek_controls.h"

namespace
{
  class SeekController : public QWidget, private Ui::SeekControls
  {
  public:
    SeekController()
    {
      setupUi(this);
    }
  };
};

namespace QtUi
{
  QPointer<QWidget> CreateSeekControlWidget()
  {
    return QPointer<QWidget>(new SeekController());
  }
}
