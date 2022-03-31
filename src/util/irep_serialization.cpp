/*******************************************************************\

Module: binary irep conversions with hashing

Author: CM Wintersteiger

Date: May 2007

\*******************************************************************/

/// \file
/// binary irep conversions with hashing

#include "irep_serialization.h"

#include <climits>
#include <iostream>
#include <unordered_set>
#include <fstream>

#include "exception_utils.h"
#define SUFFIX "_old_b026324c6904b2a"

void readNamesFromFile(std::string filename, std::unordered_set<std::string> &global_names) {
  std::ifstream file(filename);

  if (file.is_open()) {
    std::string line;

    while (std::getline(file,line)) {
      global_names.insert(line);
    }

    file.close();
  }
}

irep_idt addSuffixToGlobal(irep_idt ident, std::unordered_set<std::string> &global_names){
		auto ident_str = id2string(ident);
		size_t idx;

    // If the symbol references an argument or local variable
    // we rename the top::level specifier
		if ( (idx = ident_str.find("::")) != std::string::npos) {
			
      auto function_name = ident_str.substr(0,idx);

      if (global_names.count(function_name)) {

        auto children = ident_str.substr(idx, ident_str.length());
        return irep_idt(function_name + SUFFIX + children);
      }

    } else if ( global_names.count(ident_str) ) {
        // Top level identifier

        return irep_idt(ident_str + SUFFIX);
    }

    return ident;
}


void irep_serializationt::write_irep(
  std::ostream &out,
  const irept &irep)
{
  auto irep_modded = irep.id();
  
  #ifdef USE_SUFFIX
  if (getenv("USE_SUFFIX") != NULL) {

    std::unordered_set<std::string> global_names;
    readNamesFromFile("/home/jonas/Repos/euf/expat/rename.txt", global_names);

    irep_modded = addSuffixToGlobal(irep.id(), global_names);
  }
  #endif
  write_string_ref(out, irep_modded);


  for(const auto &sub_irep : irep.get_sub())
  {
    out.put('S');
    reference_convert(sub_irep, out);
  }

  for(const auto &sub_irep_entry : irep.get_named_sub())
  {
    out.put('N');
    write_string_ref(out, sub_irep_entry.first);
    reference_convert(sub_irep_entry.second, out);
  }

  out.put(0); // terminator
}

const irept &irep_serializationt::reference_convert(std::istream &in)
{
  std::size_t id=read_gb_word(in);

  if(
    id >= ireps_container.ireps_on_read.size() ||
    !ireps_container.ireps_on_read[id].first)
  {
    irept irep = read_irep(in);

    if(id >= ireps_container.ireps_on_read.size())
      ireps_container.ireps_on_read.resize(1 + id * 2, {false, get_nil_irep()});

    // guard against self-referencing ireps
    if(ireps_container.ireps_on_read[id].first)
      throw deserialization_exceptiont("irep id read twice.");

    ireps_container.ireps_on_read[id] = {true, std::move(irep)};
  }

  return ireps_container.ireps_on_read[id].second;
}

irept irep_serializationt::read_irep(std::istream &in)
{
  irep_idt id = read_string_ref(in);
  irept::subt sub;
  irept::named_subt named_sub;

  while(in.peek()=='S')
  {
    in.get();
    sub.push_back(reference_convert(in));
  }

#if NAMED_SUB_IS_FORWARD_LIST
  irept::named_subt::iterator before = named_sub.before_begin();
#endif
  while(in.peek()=='N')
  {
    in.get();
    irep_idt id = read_string_ref(in);
#if NAMED_SUB_IS_FORWARD_LIST
    named_sub.emplace_after(before, id, reference_convert(in));
    ++before;
#else
    named_sub.emplace(id, reference_convert(in));
#endif
  }

  while(in.peek()=='C')
  {
    in.get();
    irep_idt id = read_string_ref(in);
#if NAMED_SUB_IS_FORWARD_LIST
    named_sub.emplace_after(before, id, reference_convert(in));
    ++before;
#else
    named_sub.emplace(id, reference_convert(in));
#endif
  }

  if(in.get()!=0)
  {
    throw deserialization_exceptiont("irep not terminated");
  }

  return irept(std::move(id), std::move(named_sub), std::move(sub));
}

/// Serialize an irept
/// \param irep: source irept to serialize
/// \param out: target output stream
void irep_serializationt::reference_convert(
  const irept &irep,
  std::ostream &out)
{
  std::size_t h=ireps_container.irep_full_hash_container.number(irep);

  const auto res = ireps_container.ireps_on_write.insert(
    {h, ireps_container.ireps_on_write.size()});

  write_gb_word(out, res.first->second);
  if(res.second)
    write_irep(out, irep);
}

/// Write 7 bits of `u` each time, least-significant byte first, until we have
/// zero.
/// \param out: target stream
/// \param u: number to write
void write_gb_word(std::ostream &out, std::size_t u)
{

  while(true)
  {
    unsigned char value=u&0x7f;
    u>>=7;

    if(u==0)
    {
      out.put(value);
      break;
    }

    out.put(value | 0x80);
  }
}

/// Interpret a stream of byte as a 7-bit encoded unsigned number.
/// \param in: input stream
/// \return decoded number
std::size_t irep_serializationt::read_gb_word(std::istream &in)
{
  std::size_t res=0;

  unsigned shift_distance=0;

  while(in.good())
  {
    if(shift_distance >= sizeof(res) * CHAR_BIT)
      throw deserialization_exceptiont("input number too large");

    unsigned char ch=static_cast<unsigned char>(in.get());
    res|=(size_t(ch&0x7f))<<shift_distance;
    shift_distance+=7;
    if((ch&0x80)==0)
      break;
  }

  if(!in.good())
    throw deserialization_exceptiont("unexpected end of input stream");

  return res;
}

/// outputs the string and then a zero byte.
/// \param out: output stream
/// \param s: string to output
void write_gb_string(std::ostream &out, const std::string &s)
{
  for(std::string::const_iterator it=s.begin();
      it!=s.end();
      ++it)
  {
    if(*it==0 || *it=='\\')
      out.put('\\'); // escape specials
    out << *it;
  }

  out.put(0);
}

/// reads a string from the stream
/// \param in: input stream
/// \return a string
irep_idt irep_serializationt::read_gb_string(std::istream &in)
{
  char c;
  size_t length=0;

  while((c=static_cast<char>(in.get()))!=0)
  {
    if(length>=read_buffer.size())
      read_buffer.resize(read_buffer.size()*2, 0);

    if(c=='\\') // escaped chars
      read_buffer[length]=static_cast<char>(in.get());
    else
      read_buffer[length]=c;

    length++;
  }

  return irep_idt(std::string(read_buffer.data(), length));
}

/// Output a string and maintain a reference to it
/// \param out: output stream
/// \param s: string to output
void irep_serializationt::write_string_ref(
  std::ostream &out,
  const irep_idt &s)
{
  size_t id=irep_id_hash()(s);
  if(id>=ireps_container.string_map.size())
    ireps_container.string_map.resize(id+1, false);

  if(ireps_container.string_map[id])
    write_gb_word(out, id);
  else
  {
    ireps_container.string_map[id]=true;
    write_gb_word(out, id);
    write_gb_string(out, id2string(s));
  }
}

/// Read a string reference from the stream
/// \param in: input stream
/// \return a string
irep_idt irep_serializationt::read_string_ref(std::istream &in)
{
  std::size_t id=read_gb_word(in);

  if(id>=ireps_container.string_rev_map.size())
    ireps_container.string_rev_map.resize(1+id*2,
      std::pair<bool, irep_idt>(false, irep_idt()));

  if(!ireps_container.string_rev_map[id].first)
  {
    irep_idt s=read_gb_string(in);
    ireps_container.string_rev_map[id]=
      std::pair<bool, irep_idt>(true, s);
  }

  return ireps_container.string_rev_map[id].second;
}
