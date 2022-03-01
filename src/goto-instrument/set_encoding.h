/*******************************************************************\

Module: Set Encoding

Author:

\*******************************************************************/

/// \file
/// Set Encoding

#ifndef CPROVER_GOTO_INSTRUMENT_SET_ENCODING_H
#define CPROVER_GOTO_INSTRUMENT_SET_ENCODING_H

#include <iosfwd>

class goto_modelt;

enum class set_encoding_formatt
{
  ASCII,
  SMT2
};

void set_encoding(
  const goto_modelt &,
  set_encoding_formatt,
  std::ostream &out);

#endif // CPROVER_GOTO_INSTRUMENT_SET_ENCODING_H
