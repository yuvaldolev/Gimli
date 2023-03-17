#include "gimli/gimli_directory.h"

#include <stdio.h>

static const char *const GIMLI_DIRECTORY = "/var/lib/gimli";

const char *gimli_directory_get(void) { return GIMLI_DIRECTORY; }
