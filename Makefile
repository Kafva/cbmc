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
CXXFLAGS=-DUSE_SUFFIX


# We can get resolution using different names when we use a single file
# but if we want to use _old and current in the same cbmc invocation we
# need a better solution...

.PHONY: gen

$(CMAKE_OUT):
	@mkdir -p build
	cmake -S . -B build -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DCMAKE_C_COMPILER=/usr/bin/clang -DWITH_JBMC=OFF

$(TARGET): $(CMAKE_OUT)
	cmake --build build -- -j$(NPROC)

install: $(TARGET)
	sudo make -C build install

run: install
	USE_SUFFIX=1 goto-cc ~/Repos/oniguruma/src/st.c -o st_old.o
	USE_SUFFIX=1 cbmc --show-symbol-table st_old.o
	goto-cc ~/Repos/oniguruma/src/st.c -o st.o
	xxd st_old.o st_old.xxd
	xxd st.o st.xxd

gen: $(CMAKE_OUT)
	bear -- cmake --build build -- -j$(NPROC)

clean:
	sudo rm -rf build ./regression/goto-gcc/archives/foo.o

