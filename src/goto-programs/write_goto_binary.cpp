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

#ifdef USE_SUFFIX

#define SUFFIX "_old_b026324c6904b2a"
#define EXCLUDE_FROM "/usr"

bool is_top_level(const symbolt& sym){
	return id2string(sym.name).find("::") == std::string::npos;
}

irep_idt add_suffix(irep_idt name, bool top_level){
		auto name_str = id2string(name);
		size_t idx;

		if ( (idx = name_str.find("::")) != std::string::npos) {
			// Function parameters have symbol names on the form 'foo(arg) -> foo::arg'
			// in this case we only want to rename the top specifier (foo)
			auto new_name = name_str.substr(0,idx) + SUFFIX + \
				name_str.substr(idx, name_str.length());
			return irep_idt(new_name);

		} else if (top_level && name_str.length() > 0) {
			// If the name is a top level identifier, add a suffix
			irep_idt new_name = irep_idt(name_str + SUFFIX);
			return new_name;

		} else {

			return name;
		}
}
#endif

/// Writes a goto program to disc, using goto binary format
bool write_goto_binary(
  std::ostream &out,
  const symbol_tablet &symbol_table,
  const goto_functionst &goto_functions,
  irep_serializationt &irepconverter)
{
  // first write symbol table

  write_gb_word(out, symbol_table.symbols.size());

  for(const auto &symbol_pair : symbol_table.symbols)
  {
    // Since version 2, symbols are not converted to ireps,
    // instead they are saved in a custom binary format

    const symbolt &sym = symbol_pair.second;

    irepconverter.reference_convert(sym.type, out);
    irepconverter.reference_convert(sym.value, out);
    irepconverter.reference_convert(sym.location, out);

		auto name 				 = sym.name;
		auto base_name 		 = sym.base_name;
		auto pretty_name 	 = sym.pretty_name;
    bool is_file_local = sym.is_file_local;

		#ifdef USE_SUFFIX
    if (getenv("USE_SUFFIX") != NULL) {
      // Only add a suffix if the symbol is not defined in '/usr/*' and
      // is not a cprover built-in
      // AND is not a type specifier
      if (sym.location.as_string().find(EXCLUDE_FROM) == std::string::npos &&
          id2string(name).find("__CPROVER") == std::string::npos &&
          !sym.is_type
      ) {
        bool top_level = is_top_level(sym);
        name 					 = add_suffix(sym.name, top_level);
        base_name 		 = add_suffix(sym.base_name, top_level);
        pretty_name 	 = add_suffix(sym.pretty_name, top_level);

        // Expose all functions
        is_file_local  = false;
      }
    }
		#endif

    irepconverter.write_string_ref(out, name);
    irepconverter.write_string_ref(out, sym.module);
    irepconverter.write_string_ref(out, base_name);
    irepconverter.write_string_ref(out, sym.mode);
    irepconverter.write_string_ref(out, pretty_name);

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
      if (getenv("USE_SUFFIX") != NULL) {
        if (name_str.find("__CPROVER") == std::string::npos){
            name_str += SUFFIX;
        }
      }
      #endif

      write_gb_string(out, name_str); // name
      write_gb_word(out, fct.second.body.instructions.size()); // # instructions

      for(const auto &instruction : fct.second.body.instructions)
      {
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

        for(const auto &l_it : instruction.labels) {
          irep_idt modded_label = l_it;
          //#ifdef USE_SUFFIX
          //  if (getenv("USE_SUFFIX") != NULL) {
          //    modded_label = irep_idt(id2string(l_it) + SUFFIX);
          //  }
          //#endif
					irepconverter.write_string_ref(out, modded_label);
        }
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
