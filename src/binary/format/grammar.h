/**
 *
 * @file
 *
 * @brief  Format grammar declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "lexic_analysis.h"

namespace Binary::FormatDSL
{
  const LexicalAnalysis::TokenType DELIMITER = 0;
  const LexicalAnalysis::TokenType CONSTANT = 1;
  const LexicalAnalysis::TokenType MASK = 2;
  const LexicalAnalysis::TokenType OPERATION = 3;

  const char ANY_BYTE_TEXT = '\?';
  const char ANY_NIBBLE_TEXT = 'x';
  const char BINARY_MASK_TEXT = '%';
  const char SYMBOL_TEXT = '\'';
  const char MULTIPLICITY_TEXT = '*';

  const char RANGE_TEXT = '-';
  const char CONJUNCTION_TEXT = '&';
  const char DISJUNCTION_TEXT = '|';

  const char QUANTOR_BEGIN = '{';
  const char QUANTOR_END = '}';
  const char GROUP_BEGIN = '(';
  const char GROUP_END = ')';

  const char ANY_BIT_TEXT = 'x';
  const char ZERO_BIT_TEXT = '0';
  const char ONE_BIT_TEXT = '1';

  const char DELIMITER_TEXT = ',';

  LexicalAnalysis::Grammar::Ptr CreateFormatGrammar();
}  // namespace Binary::FormatDSL
