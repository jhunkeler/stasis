/**
* @file semaphore.c
*/
#include <stdio.h>
#include <fcntl.h>

#include "core_message.h"
#include "sem.h"
#include "utils.h"

#define STASIS_SEMAPHORE_POOL_SIZE 100
struct Semaphore **semaphores = NULL;
int semaphores_alloc_map[STASIS_SEMAPHORE_POOL_SIZE] = {0};
bool semaphore_handle_exit_ready = false;

void semaphore_handle_exit() {
    semaphore_pool_free();
}

void semaphore_pool_init() {
    if (!semaphores) {
        semaphores = calloc(STASIS_SEMAPHORE_POOL_SIZE, sizeof(*semaphores));
        memset(semaphores_alloc_map, 0, STASIS_SEMAPHORE_POOL_SIZE);
        if (semaphores == NULL) {
            SYSERROR("unable to allocate semaphore pool array");
            exit(1);
        }
    }
}

void semaphore_pool_free() {
    if (!semaphores) {
        return;
    }
    for (size_t i = 0; i < STASIS_SEMAPHORE_POOL_SIZE; i++) {
        if (semaphores_alloc_map[i]) {
            semaphore_destroy(&semaphores[i]);
        }
    }
    guard_free(semaphores);
}

static void register_semaphore(struct Semaphore **s) {
    semaphore_pool_init();
    for (size_t i = 0; i < STASIS_SEMAPHORE_POOL_SIZE; i++) {
        if (!semaphores[i]) {
            semaphores[i] = *s;
            semaphores_alloc_map[i] = 1;
            break;
        }
    }
}

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
    (*s)->sem = sem_open((*s)->name, O_CREAT, 0644, value);
    if ((*s)->sem == SEM_FAILED) {
        return -1;
    }
    SYSDEBUG("%s", (*s)->name);
    register_semaphore(s);
    if (!semaphore_handle_exit_ready) {
        atexit(semaphore_handle_exit);
        semaphore_handle_exit_ready = true;
    }

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
        for (size_t i = 0; i < STASIS_SEMAPHORE_POOL_SIZE; i++) {
            if (semaphores[i] == *s) {
                semaphores_alloc_map[i] = 0;
                break;
            }
        }
        SYSDEBUG("closing");
        if (sem_close((*s)->sem)) {
            SYSERROR("sem_close() failed: %s", strerror(errno));
        }
        (*s)->sem = NULL;

        if ((*s)->name[0] != '\0') {
            SYSDEBUG("unlinking %s", (*s)->name);
            if (sem_unlink((*s)->name)) {
                SYSERROR("sem_unlink() failed: %s", strerror(errno));
            }
        }
        (*s)->name[0] = '\0';

        guard_free(*s);
    }
}