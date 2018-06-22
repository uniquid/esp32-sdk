/**
 * @file   time.c
 *
 * @date   28/mar/2018
 * @author M. Palumbi
 */


/**
 *
 * implementation of clock_gettime()
 * needed by UID_getTime()
 *
 */

#include <sys/time.h>
#include <time.h>


int clock_gettime(clockid_t __clock_id, struct timespec *__tp)
{
    (void) __clock_id;
    struct timeval tv ;

    gettimeofday(&tv, NULL);
    __tp->tv_sec  = tv.tv_sec;
    __tp->tv_nsec = tv.tv_usec*1000;
    return 0;
}