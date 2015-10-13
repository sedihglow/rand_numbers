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

                /* headers */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <dirent.h>     /* used to go through directory information */
#include <unistd.h>     /* pathconf(dirpath, _PC_NAME_MAX); */
#include <stddef.h>     /* offsetof(type, member); */
#include <pthread.h>    /* Threadin baby, threadin */
#include "gen_rand.h"


                    /* static functions */
                    
                /* static prototypes #{{{*/
/* Pass struct* in as NULL, get time information used in generate_rand */
static int32_t time_info(struct timespec **ts, struct tm **ctm);

/* get PID information from /proc/ used in generate_rand, returns PID count,
   -1 on error, filled pidTotal with the total number all the PID's add to */
static uint32_t PID_information(const char *dirName, uint32_t *pidTotal);

/* gathering PID information */
static void* thread_PID(void *arg);

/* gathering time information */
static void* thread_time(void *arg);
/*#}}}*/

/* gathering PID information, arg = &pidTotal */
void* thread_PID(void *arg)/*#{{{*/
{
    /* gives warning if not compiled as 32-bit. uint32_t is of size 4,
       void* is of size 8 compiled as 64-bit */
    return (void*) PID_information("/proc/", arg);
} /* end thread_PID #}}} */

/* gathering time information, returns struct timespec, fills struct tm,
   void *arg should be a struct tm */
void* thread_time(void *arg)/*#{{{*/
{
    /* arg = struct tm *arg */
    struct timespec *ts = NULL; /* holds nano seconds and seconds */
    struct tm *ctm = NULL;      /* holds broken down calander information */

    if(time_info(&ts, &ctm) == -1){
        return (void*) -1;}
    
    /* only copying the values i use, dont copy the entire struct */ 
    ((struct tm*) arg) -> tm_sec = ctm -> tm_sec;
    ((struct tm*) arg) -> tm_wday = ctm -> tm_wday;
    ((struct tm*) arg) -> tm_yday = ctm -> tm_yday;

    /* free unused ctm data */
    free(ctm);
    
    return (void*) ts;
} /* end thread_time #}}} */

/* Pass struct* in as NULL, get time information used in generate_rand */
int32_t time_info(struct timespec **ts, struct tm **ctm)/*#{{{*/
{
    time_t epochSec = 0;    /* holds the time in seconds since epoch */

    /* initialize ts with CLOCK_BOOTTIME */
    *ts = (struct timespec*) malloc(sizeof(struct timespec));
    if(clock_gettime(CLOCK_BOOTTIME, *ts) == -1){
        errExit("clock_gettime: generate_rand: failed to set time.");}
    
    /* get time_t, numer of seconds since epoch */
    time(&epochSec); /* no error check, epochSec wont be NULL */

    /* send epochSec to be converted to fill ctm */
    *ctm = (struct tm*) malloc(sizeof(struct tm));
    if(gmtime_r(&epochSec, *ctm) == NULL){
        errExit("gmtime_r: generate_rand: failed to set time");}
    
    return 1;
} /* end time_info #}}} */

/* get PID information from /proc/ used in generate_rand */
uint32_t PID_information(const char *dirName, uint32_t *pidTotal)/*#{{{*/
{
    struct dirent **nameList = NULL;     /* buffer used in readdir_r. standard init */
    register int32_t numDir = 0;         /* the total number of directories */
    register uint32_t pidCount = 0;      /* number of pid process directories in /proc/ */
    register uint32_t pidConverted = 0;  /* holds the pid numer as an int */
    register uint32_t i = 0;
    
    char *endpntr = NULL;                /* error checking */
    
    /* Organize the directory into alphanumerical order, and store into
       a namelist. */
    if(scandir(dirName, &nameList, NULL, alphasort) == -1){
        errExit("PID_information, scandir");}
    
    /* traverse the list of dirent structures in nameList */
    for(i = 2; i < numDir; ++i)
    {
        /* check if d_name is a base 10 number, if it is convert */
        pidConverted = strtoul(nameList[i] -> d_name, &endpntr, 10);
        
        /* gather PID information if folder is a PID */
        if(*endpntr == '\0' && errno != ERANGE) /* sucess */
        {
            /* add the PID to the added total */
            *pidTotal += pidConverted;

            /* count the PID */
            ++pidCount;
        } /* end if */
        else
        {
            /* reset error flags */
            errno = 0;
            endpntr = NULL;
        } /* end else */
    } /* end for */
    
    /* return pidCount */
    return pidCount;
} /* end PID_information #}}} */


                    /* header functions */

/* generates a random integer and returns it, using the modVal given, returns
   -1 on error */
int32_t generate_rand(const uint32_t modVal)/*#{{{*/
{
    struct timespec *ts = NULL;     /* holds nano seconds and seconds */
    struct tm *ctm = NULL;          /* holds broken down calander information */

    register int32_t pidCount = 0;  /* number of pid process directories in /proc/ */
    register uint32_t toSeed = 0;   /* number to seed into srand */
    uint32_t pidTotal = 0;          /* holds the number that all pid's add up to */
    
    /* fill *ts and *tm */
    time_info(&ts, &ctm);

    /* gather PID information on linux system */
    pidCount = PID_information("/proc/", &pidTotal);

    /* calculate what toSeed with the values now obtained */
    toSeed = (ts -> tv_nsec >> 2) 
             + (pidTotal >> 1) 
             + (pidCount << 2) 
             + (ts -> tv_nsec + ctm -> tm_sec)
             + (ctm -> tm_wday << 3)
             + (ctm -> tm_yday ^ ctm -> tm_wday);

    if(toSeed < modVal){
        toSeed = modVal << 3;}

    /* free allocated data */
    free(ts);
    free(ctm);
    
    /* seed random, return a random int32_t, between 0 and modVal */
    srandom(toSeed); 
    return (random() % modVal);
} /* end generate_rand #}}} */

/* Threaded version of generate_rand. returns -1 on error. */
int32_t generate_rand_r(const uint32_t modVal)/*#{{{*/
{
    struct tm ctm;            /* holds broken down calander information */
    register uint32_t toSeed = 0;  /* number to seed into srand */
    uint32_t pidTotal = 0;    /* holds the number that all pid's add up to */
    pthread_t pidThread = 0;  /* thread id */
    pthread_t timeThread = 0; /* thread id */
    void *pidCount = NULL;    /* number of pid process directories in /proc/ */
    register struct timespec *ts = NULL;   /* holds nano seconds and seconds */
    void *voidts = NULL;      /* temp void pointer holding struct timespec */

    /* get PID information */   
    if(pthread_create(&pidThread, NULL, thread_PID, &pidTotal) != 0){
        errExit("pid thread");}

    /* get time information */
    if(pthread_create(&timeThread, NULL, thread_time, &ctm) != 0){
        errExit("time thread");}
    
    /* get return values of threads and make sure threads are complete */
    pthread_join(timeThread, &voidts);
    pthread_join(pidThread, &pidCount);

    ts = voidts; /* sets to actual type pointer rather than void* */
   
    /* do the rest of the algorithm after sync */
    toSeed = (ts -> tv_nsec >> 2) 
             + (pidTotal >> 1) 
             + ((uint32_t)pidCount << 2) 
             + (ts -> tv_nsec + ctm.tm_sec)
             + (ctm.tm_wday << 3)
             + (ctm.tm_yday ^ ctm.tm_wday);

    if(toSeed < modVal){
        toSeed = modVal << 3;}

    /* free allocated data */
    free(ts);
    
    /* seed random, return a random int32_t, between 0 and modVal */
    srandom(toSeed); 
    return (random() % modVal);
} /* generate_rand_r #}}} */

