#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include "rwlock.h"

/* rwl implements a reader-writer lock.
 * A reader-write lock can be acquired in two different modes, 
 * the "read" (also referred to as "shared") mode,
 * and the "write" (also referred to as "exclusive") mode.
 * Many threads can grab the lock in the "read" mode.  
 * By contrast, if one thread has acquired the lock in "write" mode, no other 
 * threads can acquire the lock in either "read" or "write" mode.
 */

const int VALID_LOCK_HEX = 0xbeef99;
const int INVALID_LOCK = -1;

//helper function
static inline int
cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *expire)
{
	

	int r; 
	if (expire != NULL)  {
		r = pthread_cond_timedwait(c, m, expire);
	} else
		r = pthread_cond_wait(c, m);
	assert(r == 0 || r == ETIMEDOUT);
       	return r;
}

//rwl_init initializes the reader-writer lock 
void
rwl_init(rwl *l)
{
	//printf("rwl_init\n");

	l->r_active = 0;
	l->r_wait = 0; 
	l->w_active = 0;
	l->w_wait =0;
	l->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init (&l->mutex, NULL);
	pthread_cond_init (&l->r_cond, NULL);
	pthread_cond_init (&l->w_cond, NULL);
	l->valid = VALID_LOCK_HEX;
}

//rwl_nwaiters returns the number of threads *waiting* to acquire the lock
//Note: it should not include any thread who has already grabbed the lock
int
rwl_nwaiters(rwl *l) 
{
	//printf("rwl_nwaiters\n");
	//rwl_rlock(l, NULL);

	int waiting = l->r_wait + l->w_wait;		

	//rwl_runlock(l);

	return waiting;
}


//rwl_rlock attempts to grab the lock in "read" mode
//if lock is not grabbed before absolute time "expire", it returns ETIMEDOUT
//else it returns 0 (when successfully grabbing the lock)
int
rwl_rlock(rwl *l, const struct timespec *expire)
{

/*
	int success = pthread_mutex_lock(&l->mutex);				
	if(success != 0) {						
		return success;
	}							

	if(l->w_active || l->w_wait) {						
		int timeout = 0;

		l->r_wait++;							

		//pthread_cleanup_push(rwl_readcleanup, (void*)l);

		while(l->w_active || l->w_wait) {				

			success = cond_timedwait(&l->r_cond, &l->mutex, expire);

			if(ETIMEDOUT == success) {				
				timeout = 1;
				break;
			}

			if(!success) {				
				break;
			}
		}

		//pthread_cleanup_pop(0);
		l->r_wait--;

		//if(timeout) 
		//	return ETIMEDOUT;
	}									

	if(0 == success) {						
		l->r_active++;
	}									

	pthread_mutex_unlock(&l->mutex);					

	return success;

*/

    if(l->w_active > 0 || l->w_wait > 0){
        pthread_mutex_lock(l->mutex);
        l->r_wait++;
        while(l->w_active > 0 || l->w_wait > 0){
            int timeout = cond_timedwait(&l->r_cond, l->mutex, expire);
            if(timeout == 0){
                l->r_wait--;
                l->r_active+= 1;
                pthread_mutex_unlock(l->mutex);
                return 0;
            }
            if(timeout == ETIMEDOUT){
                l->r_wait--;
                pthread_mutex_unlock(l->mutex);
                return timeout;
            }
        }
        l->r_wait--;
        pthread_mutex_unlock(l->mutex);
    }
    l->r_active+= 1;
	return 0;
}

//rwl_runlock unlocks the lock held in the "read" mode
void
rwl_runlock(rwl *l)
{
	/*

	if(l->valid != VALID_LOCK_HEX) {
		return;
	}

	int success = pthread_mutex_lock(&l->mutex);

	if(success != 0) {
		return;
	}

	l->r_active--;

	if(l->r_active == 0 && l->w_wait > 0) {
		success = pthread_cond_signal(&l->w_cond);
	}

	int unlocked = pthread_mutex_unlock(&l->mutex);

	return;*/


	pthread_mutex_lock(l->mutex);
	l->r_active-=1;
	pthread_mutex_unlock(l->mutex);
	pthread_cond_signal(&l->w_cond);
	return;

}


//rwl_wlock attempts to grab the lock in "write" mode
//if lock is not grabbed before absolute time "expire", it returns ETIMEDOUT
//else it returns 0 (when successfully grabbing the lock)
int
rwl_wlock(rwl *l, const struct timespec *expire)
{							
/*		
	int success = pthread_mutex_lock(&l->mutex);			


	if(0 != success) {						
		return success;
	}								

	if(l->w_active || l->r_active) {				
		int timeout = 0;

		l->w_wait++;
		

		while(l->w_active || l->r_active) {			
			success = cond_timedwait(&l->w_cond, &l->mutex, expire);

			if(ETIMEDOUT == success) {			
				timeout = 1;
				break;
			}

			if(0 != success) {				
				break;
			}
		}							

		
		l->w_wait--;

	}

	if(0 == success) {					
		l->w_active++;
	}							

	pthread_mutex_unlock(&l->mutex);				

	return success;
*/

    if(l->w_active == 1 || l->r_active > 0){
        pthread_mutex_lock(l->mutex);
        l->w_wait++;
        while(l->w_active == 1 || l->r_active > 0){
            int timeout = cond_timedwait(&l->w_cond, l->mutex, expire);
            if(timeout == 0){
                l->w_wait--;
                l->w_active = 1;
                pthread_mutex_unlock(l->mutex);
                return 0;
            }
            if(timeout == ETIMEDOUT){
                l->w_wait--;
                pthread_mutex_unlock(l->mutex);
                return timeout;
            }
        }
    }
    l->w_active = 1;
	return 0;
}

//rwl_wunlock unlocks the lock held in the "write" mode
void
rwl_wunlock(rwl *l)
{
	/*

	if(l->valid != VALID_LOCK_HEX) {
		return;
	}

	int success = pthread_mutex_lock(&l->mutex);

	if(0 != success) {
		return;
	}

	l->w_active = 0;

	if(l->r_wait > 0) {
		success = pthread_cond_broadcast(&l->r_cond);

		if(0 != success) {
			pthread_mutex_unlock(&l->mutex);
			return;
		}
	}
	else if(l->w_wait > 0) {
		success = pthread_cond_signal(&l->w_cond);
	
		if(success != 0) {
			pthread_mutex_unlock(&l->mutex);
			return;
		}
	}

	success = pthread_mutex_unlock(&l->mutex);
*/


	pthread_mutex_lock(l->mutex);
	l->w_active = 0;
	pthread_cond_signal(&l->w_cond);
	pthread_cond_broadcast(&l->r_cond);
	pthread_mutex_unlock(l->mutex);
	return;

}
