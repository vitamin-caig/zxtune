/*
Abstract:
  Update checking logic interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_CHECK_DEFINED
#define ZXTUNE_QT_UPDATE_CHECK_DEFINED

class QWidget;

namespace Update
{
  bool IsCheckingAvailable();
  void Check(QWidget& parent);
};

#endif //ZXTUNE_QT_UPDATE_CHECK_DEFINED
