#include <stdlib.h>

#include "expat.h"

const XML_LChar *XMLCALL
XML_ErrorString_old_b026324c6904b2a(enum XML_Error code);

void euf_main(){
	#ifdef CBMC
	enum XML_Error code = nondet_int();
	XML_LChar out = XML_ErrorString_old_b026324c6904b2a(code);
	XML_LChar out2 = XML_ErrorString_old_b026324c6904b2a(code);

	__CPROVER_assert(out == out2, "true");
	#endif
}
