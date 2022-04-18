/*******************************************************************\

Module: binary irep conversions with hashing

Author: CM Wintersteiger

Date: May 2007

\*******************************************************************/

/// \file
/// binary irep conversions with hashing

#ifndef CPROVER_UTIL_IREP_SERIALIZATION_H
#define CPROVER_UTIL_IREP_SERIALIZATION_H

#include <map>
#include <iosfwd>
#include <string>
#include <vector>

#include "irep_hash_container.h"
#include "irep.h"

#include <unordered_set>
#include <util/version.h>

// "irep_serialization.h" is included by both irep_serialization.cpp
// and write_goto_binary.cpp
#ifdef USE_SUFFIX
#include <string>

#define SUFFIX "_old_b026324c6904b2a"
#define RENAME_TXT "/tmp/rename.txt"

irep_idt add_suffix_to_global(irep_idt ident,
    std::unordered_set<std::string> &global_names);

#endif

void write_gb_word(std::ostream &, std::size_t);
void write_gb_string(std::ostream &, const std::string &);

class irep_serializationt
{
public:
  std::unordered_set<std::string> global_names = {};

  #ifdef USE_SUFFIX
  void read_names_from_file(std::string filename);
  #endif

  class ireps_containert
  {
  public:
    typedef std::vector<std::pair<bool, irept> > ireps_on_readt;
    ireps_on_readt ireps_on_read;

    irep_full_hash_containert irep_full_hash_container;
    typedef std::map<std::size_t, std::size_t> ireps_on_writet;
    ireps_on_writet ireps_on_write;

    typedef std::vector<bool> string_mapt;
    string_mapt string_map;

    typedef std::vector<std::pair<bool, irep_idt> > string_rev_mapt;
    string_rev_mapt string_rev_map;

    void clear()
    {
      irep_full_hash_container.clear();
      ireps_on_write.clear();
      ireps_on_read.clear();
      string_map.clear();
      string_rev_map.clear();
    }
  };

  explicit irep_serializationt(ireps_containert &ic):
    ireps_container(ic)
  {
    read_buffer.resize(1, 0);
    clear();
  };

  const irept &reference_convert(std::istream &);
  void reference_convert(const irept &irep, std::ostream &);

  irep_idt read_string_ref(std::istream &);
  void write_string_ref(std::ostream &, const irep_idt &);

  void clear() { ireps_container.clear(); }

  static std::size_t read_gb_word(std::istream &);
  irep_idt read_gb_string(std::istream &);

private:
  ireps_containert &ireps_container;
  std::vector<char> read_buffer;

  void write_irep(std::ostream &, const irept &irep);
  irept read_irep(std::istream &);
};

#endif // CPROVER_UTIL_IREP_SERIALIZATION_H
