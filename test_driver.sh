OUTDIR=/home/jonas/Repos/euf/.out
OLD_DEP=/home/jonas/.cache/euf/libexpat-bbdfcfef/expat
NEW_DEP=/home/jonas/.cache/euf/libexpat-c16300f0/expat

#OLD_LIB=/home/jonas/.cache/euf/libexpat-bbdfcfef/expat/lib/xmlparse.o
#NEW_LIB=/home/jonas/.cache/euf/libexpat-c16300f0/expat/lib/xmlparse.o

#DRIVER=/home/jonas/Repos/euf/expat/drivers/XML_ErrorString.c
#DRIVER=/home/jonas/Repos/euf/expat/drivers/getDebugLevel.c
DRIVER=/home/jonas/Repos/euf/expat/drivers/keyeq.c


OLD_LIB=/home/jonas/.cache/euf/libexpat-bbdfcfef/expat/lib/.libs/libexpat.a
NEW_LIB=/home/jonas/.cache/euf/libexpat-c16300f0/expat/lib/.libs/libexpat.a

OUTFILE=/home/jonas/Repos/euf/runner

if [ "$1" = clean ]; then
  # cbmc --show-symbol-table $OLD_LIB
  cd $NEW_DEP 
  make clean
  ./configure CC=goto-cc --host none-none-none && 
    make -j14
fi

cp $NEW_DEP/lib/expat.h $OUTDIR

cd $OLD_DEP 
make clean
./configure CC=goto-cc --host none-none-none && 
  USE_SUFFIX=1 make -j14

rm -f $OUTFILE
goto-cc -DCBMC -I $OUTDIR \
  $OLD_LIB $NEW_LIB $DRIVER \
  -o $OUTFILE


#patched_old=$(basename $OLD_LIB)
#./scripts/carver.py $OLD_LIB $patched_old
#cbmc --show-symbol-table $patched_old

EUF_ENTRYPOINT=euf_main
CBMC_OPTS=(
	--unwind 1
	--object-bits 12
)

cbmc $OUTFILE  ${CBMC_OPTS[@]} \
		--function $EUF_ENTRYPOINT \
	  --property $EUF_ENTRYPOINT.assertion.1 2>&1 \





