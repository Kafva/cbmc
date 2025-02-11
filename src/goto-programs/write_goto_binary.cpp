/*******************************************************************\

Module: Write GOTO binaries

Author: CM Wintersteiger

\*******************************************************************/

/// \file
/// Write GOTO binaries

#include "write_goto_binary.h"

#include <fstream>

#include <util/exception_utils.h>
#include <util/irep_serialization.h>
#include <util/message.h>
#include <util/symbol_table.h>

#include <goto-programs/goto_model.h>

/// Writes a goto program to disc, using goto binary format
bool write_goto_binary(
  std::ostream &out,
  const symbol_tablet &symbol_table,
  const goto_functionst &goto_functions,
  irep_serializationt &irepconverter)
{
  // first write symbol table

  write_gb_word(out, symbol_table.symbols.size());

  // Read in the list of global symbols to rename
  // Note that we read this file even when SUFFIX_ENV_FLAG
  // is unset since global names need to be stripped
  // of the is_file_local flag regardless of if
  // we are adding suffixes
  #ifdef USE_SUFFIX
    irepconverter.read_names_from_file(RENAME_TXT);
  #endif

  for(const auto &symbol_pair : symbol_table.symbols)
  {
    // Since version 2, symbols are not converted to ireps,
    // instead they are saved in a custom binary format

    const symbolt &sym = symbol_pair.second;

    // `reference_convert` calls can store strings in the '.data' section
    // of the resulting binary using `write_irep()`. If these strings correspond
    // to global symbol names they need to be renamed.
    // We therefore inspect the list of global names in _ALL_ calls of:
    //  src/util/irep_serialization.cpp:irep_serializationt::write_string_ref()
    irepconverter.reference_convert(sym.type, out);
    irepconverter.reference_convert(sym.value, out);
    irepconverter.reference_convert(sym.location, out);

    // Ensure that every function is callable from another TU
    // **** APPLIES REGARDLESS of `SUFFIX_ENV_FLAG` ****
    // Note: applying this for all identifiers can cause issues
    // when the same identifier is defined across multiple TUs
    //
    // We therefore limit this change to a list of provided names
    bool is_file_local = sym.is_file_local;
    if (irepconverter.global_names.count(id2string(sym.name))) {
      is_file_local  = false;
    }

    irepconverter.write_string_ref(out, sym.name);
    irepconverter.write_string_ref(out, sym.module);
    irepconverter.write_string_ref(out, sym.base_name);
    irepconverter.write_string_ref(out, sym.mode);
    irepconverter.write_string_ref(out, sym.pretty_name);

    write_gb_word(out, 0); // old: sym.ordering

    unsigned flags=0;
    flags = (flags << 1) | static_cast<int>(sym.is_weak);
    flags = (flags << 1) | static_cast<int>(sym.is_type);
    flags = (flags << 1) | static_cast<int>(sym.is_property);
    flags = (flags << 1) | static_cast<int>(sym.is_macro);
    flags = (flags << 1) | static_cast<int>(sym.is_exported);
    flags = (flags << 1) | static_cast<int>(sym.is_input);
    flags = (flags << 1) | static_cast<int>(sym.is_output);
    flags = (flags << 1) | static_cast<int>(sym.is_state_var);
    flags = (flags << 1) | static_cast<int>(sym.is_parameter);
    flags = (flags << 1) | static_cast<int>(sym.is_auxiliary);
    flags = (flags << 1) | static_cast<int>(false); // sym.binding;
    flags = (flags << 1) | static_cast<int>(sym.is_lvalue);
    flags = (flags << 1) | static_cast<int>(sym.is_static_lifetime);
    flags = (flags << 1) | static_cast<int>(sym.is_thread_local);
    flags = (flags << 1) | static_cast<int>(is_file_local);
    flags = (flags << 1) | static_cast<int>(sym.is_extern);
    flags = (flags << 1) | static_cast<int>(sym.is_volatile);

    write_gb_word(out, flags);
  }

  // now write functions, but only those with body

  unsigned cnt=0;
  for(const auto &gf_entry : goto_functions.function_map)
  {
    if(gf_entry.second.body_available())
      cnt++;
  }

  write_gb_word(out, cnt);

  for(const auto &fct : goto_functions.function_map)
  {
    if(fct.second.body_available())
    {
      // Since version 2, goto functions are not converted to ireps,
      // instead they are saved in a custom binary format

      auto name_str = id2string(fct.first);

      #ifdef USE_SUFFIX
      if (getenv(SUFFIX_ENV_FLAG) != NULL) {
        if (irepconverter.global_names.count(name_str)) {
            name_str += SUFFIX;
        }
      }
      #endif

      // Bypasses, write_string_ref(), a suffix must be explicitly added
      write_gb_string(out, name_str); // name
      write_gb_word(out, fct.second.body.instructions.size()); // # instructions

      for(const auto &instruction : fct.second.body.instructions)
      {
        // Replace any occurences of global names in the reference conversion
        irepconverter.reference_convert(instruction.get_code(), out);
        irepconverter.reference_convert(instruction.source_location(), out);
        write_gb_word(out, (long)instruction.type());

        const auto condition =
          instruction.has_condition() ? instruction.condition() : true_exprt();
        irepconverter.reference_convert(condition, out);

        write_gb_word(out, instruction.target_number);

        write_gb_word(out, instruction.targets.size());

        for(const auto &t_it : instruction.targets)
          write_gb_word(out, t_it->target_number);

        write_gb_word(out, instruction.labels.size());

        // This iterates over goto labels in the original source code
        for(const auto &l_it : instruction.labels)
          irepconverter.write_string_ref(out, l_it);
      }
    }
  }

  // irepconverter.output_map(f);
  // irepconverter.output_string_map(f);

  return false;
}

/// Writes a goto program to disc
bool write_goto_binary(
  std::ostream &out,
  const goto_modelt &goto_model,
  int version)
{
  return write_goto_binary(
    out,
    goto_model.symbol_table,
    goto_model.goto_functions,
    version);
}

/// Writes a goto program to disc
bool write_goto_binary(
  std::ostream &out,
  const symbol_tablet &symbol_table,
  const goto_functionst &goto_functions,
  int version)
{
  // header
  out << char(0x7f) << "GBF";
  write_gb_word(out, version);

  irep_serializationt::ireps_containert irepc;
  irep_serializationt irepconverter(irepc);

  if(version < GOTO_BINARY_VERSION)
    throw invalid_command_line_argument_exceptiont(
      "version " + std::to_string(version) + " no longer supported",
      "supported version = " + std::to_string(GOTO_BINARY_VERSION));
  else if(version > GOTO_BINARY_VERSION)
    throw invalid_command_line_argument_exceptiont(
      "unknown goto binary version " + std::to_string(version),
      "supported version = " + std::to_string(GOTO_BINARY_VERSION));
  else
    return write_goto_binary(out, symbol_table, goto_functions, irepconverter);
}

/// Writes a goto program to disc
bool write_goto_binary(
  const std::string &filename,
  const goto_modelt &goto_model,
  message_handlert &message_handler)
{
  std::ofstream out(filename, std::ios::binary);

  if(!out)
  {
    messaget message(message_handler);
    message.error() << "Failed to open '" << filename << "'";
    return true;
  }

  return write_goto_binary(out, goto_model);
}
