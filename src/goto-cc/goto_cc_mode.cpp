/*******************************************************************\

Module: Command line option container

Author: CM Wintersteiger, 2006

\*******************************************************************/

/// \file
/// Command line option container

#include "goto_cc_mode.h"

#include <iostream>

#ifdef _WIN32
#define EX_OK 0
#define EX_USAGE 64
#define EX_SOFTWARE 70
#else
#include <sysexits.h>
#endif

#include <util/exception_utils.h>
#include <util/message.h>
#include <util/parse_options.h>
#include <util/version.h>

#include "goto_cc_cmdline.h"

/// constructor
goto_cc_modet::goto_cc_modet(
  goto_cc_cmdlinet &_cmdline,
  const std::string &_base_name,
  message_handlert &_message_handler)
  : cmdline(_cmdline), base_name(_base_name), message_handler(_message_handler)
{
  register_languages();
}

/// constructor
goto_cc_modet::~goto_cc_modet()
{
}

/// display command line help
void goto_cc_modet::help()
{
  // clang-format off
  std::cout << '\n' << banner_string("goto-cc", CBMC_VERSION) << '\n'
            << align_center_with_border("Copyright (C) 2006-2018") << '\n'
            << align_center_with_border("Daniel Kroening, Michael Tautschnig,") << '\n' // NOLINT(*)
            << align_center_with_border("Christoph Wintersteiger") << '\n'
            <<
  "\n";

  #ifdef USE_SUFFIX
    std::cout
      << align_center_with_border("Compiled with USE_SUFFIX") << '\n'
      << align_center_with_border((getenv(SUFFIX_ENV_FLAG) != NULL ? 
          "Renaming active: TRUE" : 
          "Renaming active: FALSE"
         )) 
      << "\n";
  #endif

  help_mode();

  std::cout <<
  "Usage:                       Purpose:\n"
  "\n"
  " --verbosity #               verbosity level\n"
  " --function name             set entry point to name\n"
  " --native-compiler cmd       command to invoke as preprocessor/compiler\n"
  " --native-linker cmd         command to invoke as linker\n"
  " --native-assembler cmd      command to invoke as assembler (goto-as only)\n"
  " --print-rejected-preprocessed-source file\n"
  "                             copy failing (preprocessed) source to file\n"
  " --object-bits               number of bits used for object addresses\n"
  "\n";
  // clang-format on
}

/// starts the compiler
/// \return error code
int goto_cc_modet::main(int argc, const char **argv)
{
  if(cmdline.parse(argc, argv))
  {
    usage_error();
    return EX_USAGE;
  }

  try
  {
    return doit();
  }

  catch(const char *e)
  {
    messaget log{message_handler};
    log.error() << e << messaget::eom;
    return EX_SOFTWARE;
  }

  catch(const std::string &e)
  {
    messaget log{message_handler};
    log.error() << e << messaget::eom;
    return EX_SOFTWARE;
  }

  catch(int)
  {
    return EX_SOFTWARE;
  }

  catch(const std::bad_alloc &)
  {
    messaget log{message_handler};
    log.error() << "Out of memory" << messaget::eom;
    return EX_SOFTWARE;
  }
  catch(const cprover_exception_baset &e)
  {
    messaget log{message_handler};
    log.error() << e.what() << messaget::eom;
    return EX_SOFTWARE;
  }
}

/// prints a message informing the user about incorrect options
/// \return none
void goto_cc_modet::usage_error()
{
  std::cerr << "Usage error!\n\n";
  help();
}
