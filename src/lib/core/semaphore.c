/**
* @file semaphore.c
*/
#include <stdio.h>
#include <fcntl.h>

#include "core_message.h"
#include "sem.h"
#include "utils.h"

int semaphore_created_by_this_process = 0;

int semaphore_init(struct Semaphore **s, const char *name, const int value) {
    if (*s == NULL) {
        *s = calloc(1, sizeof(**s));
    }
#if defined(STASIS_OS_DARWIN)
    // see: sem_open(2)
    const size_t max_namelen = PSEMNAMLEN;
#else
    // see: sem_open(3)
    const size_t max_namelen = STASIS_NAME_MAX;
#endif
    snprintf((*s)->name, max_namelen, "/%s", name);
    (*s)->sem = sem_open((*s)->name, O_CREAT | O_EXCL, 0600, value);
    if ((*s)->sem == SEM_FAILED) {
        if (errno == EEXIST) {
            SYSWARN( "named semaphore already exists: %s", (*s)->name);
            (*s)->sem = sem_open((*s)->name, 0);
            if ((*s)->sem == SEM_FAILED) {
                SYSERROR("sem_open() failed: %s", strerror(errno));
                exit(1);
            }
        } else {
            SYSERROR("sem_open() failed: %s", strerror(errno));
            exit(1);
        }
    }
    SYSDEBUG("%s", (*s)->name);

    return 0;
}

int semaphore_wait(struct Semaphore *s) {
#if defined(STASIS_SEMAPHORE_DEBUG)
    int sgv_value = 0;
    int sgv_ret = sem_getvalue(s->sem, &sgv_value);
    SYSDEBUG("sem_getvalue() returned %d, value %d", sgv_ret, sgv_value);
#endif
    const int status = sem_wait(s->sem);
#if defined(STASIS_SEMAPHORE_DEBUG)
    SYSDEBUG("returning %d", status);
#endif
    return status;
}

int semaphore_post(struct Semaphore *s) {
#if defined(STASIS_SEMAPHORE_DEBUG)
    int sgv_value = 0;
    int sgv_ret = sem_getvalue(s->sem, &sgv_value);
    SYSDEBUG("sem_getvalue() returned %d, value %d", sgv_ret, sgv_value);
#endif
    const int status = sem_post(s->sem);
#if defined(STASIS_SEMAPHORE_DEBUG)
    SYSDEBUG("returning %d", status);
#endif
    return status;
}

void semaphore_destroy(struct Semaphore **s) {
    if (*s) {
        SYSDEBUG("closing");
        if (sem_close((*s)->sem)) {
            SYSERROR("sem_close() failed: %s", strerror(errno));
        }
        (*s)->sem = NULL;

        if ((*s)->name[0] != '\0' && semaphore_created_by_this_process) {
            SYSDEBUG("unlinking %s", (*s)->name);
            if (sem_unlink((*s)->name)) {
                SYSERROR("sem_unlink() failed: %s", strerror(errno));
            }
        }
        (*s)->name[0] = '\0';

        guard_free(*s);
    }
}