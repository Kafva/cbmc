#===== http://cprover.diffblue.com/group__goto-programs.html =====#

# src/goto-programs/goto-convert.cpp
# src/goto-programs/write_goto_binary:write_goto_binary()


#===   initialize_from_source_files(sources, options, language_files, goto_model.symbol_table, message_handler);
#===		Ensure that symtab is changed here..?																																===#
# Earlier: src/goto-programs/initialize_goto_model.cpp:205:

#=== src/ansi-c/ansi_c_language.cpp
# Only the language.typecheck() function uses the sym table
#		ansi_c_languaget::typecheck():103


# Earliest stage: src/ansi-c/expr2c.cpp
# This is where we get a panic

# The initialize_goto_model() creates a new `goto_model`
# The functions and symbol table of the goto_model are manipulated in `goto_convert`

# goto_convert.cpp should be a good interface since it is meant to perform transformations...


# The SUFFIX to use is configured inside src/util/namespace.h
CMAKE_OUT=cbmc/build/Makefile
TARGET=cbmc/build/bin/goto-cc
NPROC=$(shell printf $$((`nproc` - 1)) )
CXXFLAGS=-DUSE_SUFFIX -g
#CXXFLAGS=

INPUT=~/.cache/euf/libexpat-bbdfcfef/expat/lib/xmlparse
BASE_INPUT=xmlparse

# We can get resolution using different names when we use a single file
# but if we want to use _old and current in the same cbmc invocation we
# need a better solution...

# http://cprover.diffblue.com/classirept.html
#
# src/util/irep_serialization.cpp:	reference_convert()

# THIS: irep_serializationt::write_irep()

# !!: There is still one textual reference left for every function that has not been renamed...

.PHONY: gen

$(CMAKE_OUT):
	@mkdir -p build
	cmake -S . -B build -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DCMAKE_C_COMPILER=/usr/bin/clang -DWITH_JBMC=OFF

$(TARGET): $(CMAKE_OUT)
	cmake --build build -- -j$(NPROC)

install: $(TARGET)
	sudo make -C build install


gdb: install
	USE_SUFFIX=1 gdb --args goto-cc $(INPUT).c -o $(INPUT)_old.o

compile: install
	USE_SUFFIX=1 goto-cc $(INPUT).c -o $(INPUT)_old.o
	USE_SUFFIX=1 cbmc --show-symbol-table $(INPUT)_old.o
	#USE_SUFFIX=1 cbmc --show-goto-functions $(INPUT)_old.o
	goto-cc $(INPUT).c -o $(INPUT).o


driver: install
	../scripts/cbmc_test.sh

gen: $(CMAKE_OUT)
	bear -- cmake --build build -- -j$(NPROC)

clean:
	sudo rm -rf build ./regression/goto-gcc/archives/foo.o

