/*
Abstract:
  Playback options widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED
#define ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED

//common includes
#include <parameters.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackOptions : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackOptions(QWidget& parent);
public:
  //creator
  static PlaybackOptions* Create(QWidget& parent, Parameters::Container& params);
};

#endif //ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED
