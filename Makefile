#===== http://cprover.diffblue.com/group__goto-programs.html =====#
# Note that the project needs to be compiled with -DUSE_SUFFIX for any of the modifications
# to be included. To actually activate them the same string needs to be set in the environment
# when invoking cbmc, goto-cc or another cprover tool.

#== src/goto-programs/write_goto_binary:write_goto_binary() ==#
# Override the symbol table and function body creation, adding suffixes to all global symbols

#==	src/util/irep_serialization.cpp:irep_serializationt::write_irep() ==#
# Hook into the write_irep() function to add suffixes
# to string symbols in the '.data' of the goto-bin
# 	http://cprover.diffblue.com/classirept.html

CMAKE_OUT=cbmc/build/Makefile
TARGET=cbmc/build/bin/goto-cc
NPROC=$(shell printf $$((`nproc` - 1)) )
CXXFLAGS=-DUSE_SUFFIX
#CXXFLAGS=


#INPUT=~/.cache/euf/oniguruma-65a9b1aa/st
#BASE_INPUT=st
#RENAME_TXT=~/Repos/euf/tests/data/oni_rename.txt

INPUT=~/.cache/euf/libexpat-bbdfcfef/expat/lib/xmlparse
BASE_INPUT=xmlparse
RENAME_TXT=~/Repos/euf/examples/expat/rename.txt

.PHONY: gen

$(CMAKE_OUT):
	@mkdir -p build
	cmake -S . -B build -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DCMAKE_C_COMPILER=/usr/bin/clang -DWITH_JBMC=OFF

$(TARGET): $(CMAKE_OUT)
	cmake --build build -- -j$(NPROC)

install: $(TARGET)
	sudo make -C build install

usb: install
	@EXIT=false PROJ=libusb \
	FILE=libusb/core.c \
	SHOW_DIFF=false \
	CONTEXT_LINES=0 \
	SILENT=false \
	../scripts/test_harness.sh \
	examples/libusb.json \
	libusb_attach_kernel_driver

#  - - - - - - - - - - - - - - - - - - - - #

run: install
	cp $(RENAME_TXT) /tmp/rename.txt
	USE_SUFFIX=1 goto-cc $(INPUT).c -o $(INPUT)_old.o
	cbmc --show-symbol-table $(INPUT)_old.o
	#USE_SUFFIX=1 cbmc --show-goto-functions $(INPUT)_old.o
	goto-cc $(INPUT).c -o $(INPUT).o

# Shows why we need to hook into the creation, not all references are resolved
compare: install
	printf "lookup\nXML_ErrorString\n" > /tmp/rename.txt
	./carver.py examples/xmlparse.gb examples/xmlparse_carved.gb
	USE_SUFFIX=1 goto-cc examples/xmlparse.gb -o examples/xmlparse_new.gb
	#goto-cc -DCBMC examples/XML_ErrorString.c examples/xmlparse_carved.gb -o runner
	#cbmc --function euf_main ./runner

example: install
	cp $(RENAME_TXT) /tmp/rename.txt
	USE_SUFFIX=1 goto-cc $(INPUT).c -o $(INPUT)_old.o
	cbmc  --show-goto-functions $(INPUT)_old.o | grep --color=always -A100 "^onig_st_add_direct"

gdb: install
	USE_SUFFIX=1 gdb --args goto-cc $(INPUT).c -o $(INPUT)_old.o

gen: $(CMAKE_OUT)
	bear -- cmake --build build -- -j$(NPROC)

clean_tests:
	sudo rm -f ./regression/goto-gcc/archives/foo.o

clean: clean_tests
	sudo rm -rf build 

