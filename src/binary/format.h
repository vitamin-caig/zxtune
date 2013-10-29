/**
*
* @file
*
* @brief  Binary data format interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data.h>

namespace Binary
{
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

  //! Data format description
  class Format
  {
  public:
    typedef boost::shared_ptr<const Format> Ptr;
    virtual ~Format() {}

    //! @brief Check if input data is data format
    //! @param data Data to be checked
    //! @return true if data comply format
    virtual bool Match(const Data& data) const = 0;
    //! @brief Search for matched offset in input data
    //! @param data Data to be checked
    //! @return Offset of matched data or size if not found
    //! @invariant return value is always > 0
    virtual std::size_t NextMatchOffset(const Data& data) const = 0;

    // Factory based on text pattern
    static Ptr Create(const std::string& pattern, std::size_t minSize = 0);
  };

  Format::Ptr CreateCompositeFormat(Format::Ptr header, Format::Ptr footer, std::size_t minFooterOffset, std::size_t maxFooterOffset);
}
