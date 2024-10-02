/**
 *
 * @file
 *
 * @brief  Binary data format factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// library includes
#include <binary/format.h>

namespace Binary
{
  // clang-format off

  //Text pattern format (in regular expression notation):
  //  \?           - any byte
  //  [0-9a-fx]{2} - match byte/nibble
  //  %[01x]{8}    - match byte by bits
  //  '.           - match symbol
  //  [0-9]{2}-[0-9]{2} - match byte by range
  //  \{[0-9]+\}   - quantor
  //  \(.*\)       - group subpatterns (useful with quantors), may be nested
  //  \*[0-9]+     - match multiplicity (dec)
  //  smth & smth  - conjunction
  //  smth | smth  - disjunction

  /* Pattern grammar

  pattern      ::= sequence | 0
  sequence     ::= multi_match | sequence multi_match | sequence space | space sequence | group | group '{' quantor '}'
  group        ::= '(' sequence ')'
  multi_match  ::= single_match | single_match '-' single_match | '?' | any_nibble any_nibble | '%' any_bit x 8 | '*' quantor
  multi_match  ::= multi_match '|' multi_match | multi_match '&' multi_match
  single_match ::= nibble nibble | '%' bit x 8 | ''' any_char
  any_nibble   ::= nibble | 'x'
  any_bit      ::= bit | 'x'
  nibble       ::= digit | 'a'..'f' | 'A'..'F'
  bit          ::= '0' | '1'
  digit        ::= '0'..'9'
  quantor      ::= digit | quantor digit
  space        ::= space_char | space space_char
  space_char   ::= ' ' | '\t' | '\n' | '\r'
  */

  // clang-format on

  Format::Ptr CreateFormat(StringView pattern);
  Format::Ptr CreateFormat(StringView pattern, std::size_t minSize);
  Format::Ptr CreateCompositeFormat(Format::Ptr header, Format::Ptr footer, std::size_t minFooterOffset,
                                    std::size_t maxFooterOffset);
  Format::Ptr CreateMatchOnlyFormat(StringView pattern);
  Format::Ptr CreateMatchOnlyFormat(StringView pattern, std::size_t minSize);
}  // namespace Binary
