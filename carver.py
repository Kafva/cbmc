#!/usr/bin/env python3
#----------------------------#
#== http://cprover.diffblue.com/group__goto-programs.html#Binary%20Representation ==#

# Check symbol replacement status:
#		cbmc --show-symbol-table goto/xmlparse.o

import os, sys

SUFFIX: str = "_old_b026324c6904b2a" # Do not change, hardcoded in CBMC fork

def die(msg: str, code = 0):
    print(msg)
    exit(code)

if len(sys.argv) < 2:
    die(f"{os.path.basename(__file__)} <input.gb> <output.gb>")

def decode_int_7bit(b_arr: bytearray):
    '''
     Interpret a stream of byte as a 7-bit encoded unsigned number.
        src/util/irep_serialisation.cpp
    '''
    res=0
    shift_distance=0

    for b in b_arr:
      res = res | (b & 0x7f)  << shift_distance
      shift_distance+=7

      if b & 0x80 == 0:
        break

    return res

def encode_int_7bit(u: int):
    '''
    Write 7 bits of `u` each time, least-significant byte first, until we have
    zero.
        src/util/irep_serialisation.cpp
    '''
    out = bytearray()

    while True:
        sub = u & 0x7f;
        u = u >> 7;
        if u==0:
            out.append(sub)
            break;
    out.append( sub | 0x80 )
    return out

content = b''

# See: ./src/goto-program/read_bin_goto_object.cpp for in depth view
# of the fields
with open(sys.argv[1], mode='rb') as f:
    if (content := f.read(4)) != b'\x7fGBF': # GBF header
        die("Invalid input file")

    content += f.read(1) # Version number

    # Number of symbols in the symbol table, should be the same as:
    #   `cbmc --show-symbol-table goto/xmlrole.o | grep -c "^Symbol\."`
    symbol_cnt_bytes = f.read(2)
    symbol_count = decode_int_7bit(bytearray(symbol_cnt_bytes))
    content += symbol_cnt_bytes

    # Array of symbols
    blob = f.read()

    global_names =[]
    with open("../expat/rename.csv", mode="r", encoding="utf8") as f:
        for line in f.readlines():
            first_index = line.index(";")
            second_index = line.index(";", first_index+1)
            global_names.append( line[first_index+1 : second_index] )

    # A lot of the textual symbols are terminated with a null character and start 
    # with \x86 \x01
    #
    # The string-ref used by instructions in the function body is probably the one that
    # we cant replace in cbmc (and the one without proper terimination)
    #
    # We also want to replace occurences on the form foo::arg which correspond
    # to foo(arg) in the source code as foo_SUFFIX::arg
    #
    # We do not want to replace typenames and "tag-(name)" occurences which match
    # the global names as substrings
    for name in global_names:
        blob = blob.replace(
            b'\x86\x01'+name.encode('ascii')+b'\x00',
            b'\x86\x01'+(name+SUFFIX).encode('ascii')+b'\x00'
        )
        blob = blob.replace(
            b'\x86\x01'+name.encode('ascii')+b'::',
            b'\x86\x01'+(name+SUFFIX).encode('ascii')+b'::'
        )

with open(sys.argv[2], mode='wb') as f:
    f.write(content + blob)
