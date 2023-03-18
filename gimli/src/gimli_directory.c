#include "gimli/gimli_directory.h"

#include <stdio.h>

/* static const char *const GIMLI_DIRECTORY = "/var/lib/gimli"; */
static const char *const GIMLI_DIRECTORY = "/Library/gimli";

const char *gimli_directory_get(void) { return GIMLI_DIRECTORY; }
