/*
Abstract:
  Format syntax declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BINARY_FORMAT_SYNTAX_H
#define BINARY_FORMAT_SYNTAX_H

//common includes
#include <types.h>

namespace Binary
{
  class FormatTokensVisitor
  {
  public:
    virtual ~FormatTokensVisitor() {}

    virtual void Value(const std::string& val) = 0;
    virtual void GroupStart() = 0;
    virtual void GroupEnd() = 0;
    virtual void Quantor(uint_t count) = 0;
    virtual void Operation(const std::string& op) = 0;
  };

  void ParseFormatNotation(const std::string& notation, FormatTokensVisitor& visitor);
}

#endif //BINARY_FORMAT_SYNTAX_H
