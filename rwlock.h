#ifndef RWLOCK_H
#define RWLOCK_H

typedef struct {
	pthread_mutex_t     *mutex;
	pthread_cond_t      r_cond;
	pthread_cond_t      w_cond;
	int                 valid;
	int                 r_active;
	int                 w_active;
	int                 r_wait;
	int                 w_wait;
}rwl;

void rwl_init(rwl *l);
int rwl_nwaiters(rwl *l);
int rwl_rlock(rwl *l, const struct timespec *expire);
void rwl_runlock(rwl *l);
int rwl_wlock(rwl *l, const struct timespec *expire);
void rwl_wunlock(rwl *l);

#endif
