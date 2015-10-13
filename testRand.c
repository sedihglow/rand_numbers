/* tests the random number generator yo */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "gen_rand.h"



#define N_TIMES 6666

int main()
{
    
    register uint32_t randomed = 0; /* resulting random number */
    register uint32_t i = 0;
    
    /* modval = 50 */
    for(;i < N_TIMES; ++i)
    {
        randomed = generate_rand(50);
    } /* end for */

    printf("\nrandomed: %"PRIu32"\n", randomed);



    return 0;

} /* end main */
