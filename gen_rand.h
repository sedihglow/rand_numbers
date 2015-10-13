/* 
    written by: James Ross

    If you have it, ask before using it. Thx.

    generates a random number useing the following information.

    - nano seconds (timespec)
    - time_t in seconds (tm)
    - day of the week
    - day of the year
    using the /proc/ directory
    - count of PID's currently in /proc/
    - the total number the PID's add up to

    generate_rand() is not threaded.
    generate_rand_r() is multi-threaded. (gets time and directory info at once)

    NOTES: returning -1 when readdir_r fails could mean multiple things. Not
           all read failures may be bad. Still, this current version closes the
           program with an error message if readdir_r fails at all.
*/

#ifndef __GEN_RAND_H__
#define __GEN_RAND_H__

                    /* headers */
#include <inttypes.h>
#include "err_handle.h"

                    /* prototypes */
/* not threaded, uses thread safe functions */
int32_t generate_rand(const uint32_t modVal);

/* threaded, uses thread safe functions */
int32_t generate_rand_r(const uint32_t modVal);

#endif
