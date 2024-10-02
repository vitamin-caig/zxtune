/**
 *
 * @file
 *
 * @brief  Format syntax helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// std includes
#include <memory>

namespace Binary::FormatDSL
{
  class FormatTokensVisitor
  {
  public:
    using Ptr = std::unique_ptr<FormatTokensVisitor>;
    virtual ~FormatTokensVisitor() = default;

    virtual void Match(StringView val) = 0;
    virtual void GroupStart() = 0;
    virtual void GroupEnd() = 0;
    virtual void Quantor(uint_t count) = 0;
    virtual void Operation(StringView op) = 0;
  };

  void ParseFormatNotation(StringView notation, FormatTokensVisitor& visitor);
  void ParseFormatNotationPostfix(StringView notation, FormatTokensVisitor& visitor);
  FormatTokensVisitor::Ptr CreatePostfixSyntaxCheckAdapter(FormatTokensVisitor& visitor);
}  // namespace Binary::FormatDSL
