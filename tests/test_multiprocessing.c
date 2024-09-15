#include "testing.h"

static struct MultiProcessingPool *pool;
char *commands[] = {
        "true",
        "uname -a",
        "/bin/echo hello world",
};

void test_mp_pool_init() {
    STASIS_ASSERT((pool = mp_pool_init("mypool", "mplogs")) != NULL, "Pool initialization failed");
    STASIS_ASSERT_FATAL(pool != NULL, "Should not be NULL");
    STASIS_ASSERT(pool->num_alloc == MP_POOL_TASK_MAX, "Wrong number of default records");
    STASIS_ASSERT(pool->num_used == 0, "Wrong number of used records");
    STASIS_ASSERT(strcmp(pool->log_root, "mplogs") == 0, "Wrong log root directory");
    STASIS_ASSERT(strcmp(pool->ident, "mypool") == 0, "Wrong identity");
    int data_bad = 0;
    char *data = calloc(pool->num_alloc, sizeof(*pool->task));
    for (size_t i = 0; i < pool->num_alloc * sizeof(*pool->task); i++) {
        if (data[i] != 0) {
            data_bad = 1;
            break;
        }
    }
    free(data);
    STASIS_ASSERT(data_bad == 0, "Task array should be zeroed");
    mp_pool_free(&pool);
}

void test_mp_task() {
    pool = mp_pool_init("mypool", "mplogs");

    if (pool) {
        for (size_t i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
            struct MultiProcessingTask *task;
            char task_name[100] = {0};
            sprintf(task_name, "mytask%zu", i);
            STASIS_ASSERT_FATAL((task = mp_task(pool, task_name, commands[i])) != NULL, "Task should not be NULL");
            STASIS_ASSERT(task->pid != 0, "PID should be non-zero at this point");
            STASIS_ASSERT(task->parent_pid != MP_POOL_PID_UNUSED, "Parent PID should be non-zero");
            STASIS_ASSERT(task->status == 0, "Status should be zero");
            STASIS_ASSERT(strcmp(task->ident, task_name) == 0, "Wrong task identity");
            STASIS_ASSERT(strstr(task->log_file, pool->log_root) != NULL, "Log file path must be in log_root");
            STASIS_ASSERT(task->gate != NULL, "Semaphore should be initialized");
        }
    }
}

void test_mp_pool_join() {
    STASIS_ASSERT(mp_pool_join(pool, get_cpu_count(), 0) == 0, "Pool tasks should have not have failed");
    for (size_t i = 0; i < pool->num_used; i++) {
        struct MultiProcessingTask *task = &pool->task[i];
        STASIS_ASSERT(task->pid == MP_POOL_PID_UNUSED, "Task should be marked as unused");
        STASIS_ASSERT(task->status == 0, "Task should have succeeded");
    }
}

void test_mp_pool_free() {
    mp_pool_free(&pool);
    STASIS_ASSERT(pool == NULL, "Should be NULL");
}

int main(int argc, char *argv[]) {
    STASIS_TEST_BEGIN_MAIN();
    STASIS_TEST_FUNC *tests[] = {
        test_mp_pool_init,
        test_mp_task,
        test_mp_pool_join,
        test_mp_pool_free,
    };
    STASIS_TEST_RUN(tests);
    STASIS_TEST_END_MAIN();
}