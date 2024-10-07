#ifndef _ASH_ENV_H
#define _ASH_ENV_H

#include <stdlib.h>
#include <string.h>

#define GET_ENV(e) strdup(getenv(e))

#define env_home GET_ENV("HOME")

#endif
