#include "system.h"
#include "core.h"

int shell(struct Process *proc, char *args) {
    struct Process selfproc;
    pid_t status;
    status = 0;
    errno = 0;

    if (!proc) {
        // provide our own proc structure
        // albeit not accessible to the user
        memset(&selfproc, 0, sizeof(selfproc));
        proc = &selfproc;
    }

    if (!args) {
        proc->returncode = -1;
        return -1;
    }

    FILE *tp = NULL;
    char *t_name = xmkstemp(&tp, "w");
    if (!t_name || !tp) {
        return -1;
    }

    fprintf(tp, "#!/bin/bash\n%s\n", args);
    fflush(tp);
    fclose(tp);

    // Set the script's permissions so that only the calling user can use it
    // This should help prevent eavesdropping if keys are applied in plain-text
    // somewhere.
    chmod(t_name, 0700);

    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (pid == 0) {
        FILE *fp_out = NULL;
        FILE *fp_err = NULL;

        if (strlen(proc->f_stdout)) {
            fp_out = freopen(proc->f_stdout, "w+", stdout);
            if (!fp_out) {
                fprintf(stderr, "Unable to redirect stdout to %s: %s\n", proc->f_stdout, strerror(errno));
                exit(1);
            }
        }

        if (strlen(proc->f_stderr)) {
            if (!proc->redirect_stderr) {
                fp_err = freopen(proc->f_stderr, "w+", stderr);
                if (!fp_err) {
                    fprintf(stderr, "Unable to redirect stderr to %s: %s\n", proc->f_stdout, strerror(errno));
                    exit(1);
                }
            }
        }

        if (proc->redirect_stderr) {
            if (fp_err) {
                fclose(fp_err);
                fclose(stderr);
            }
            if (dup2(fileno(stdout), fileno(stderr)) < 0) {
                fprintf(stderr, "Unable to redirect stderr to stdout: %s\n", strerror(errno));
                exit(1);
            }
        }

        return execl("/bin/bash", "bash", "--norc", t_name, (char *) NULL);
    } else {
        if (waitpid(pid, &status, WUNTRACED) > 0) {
            if (WIFEXITED(status) && WEXITSTATUS(status)) {
                if (WEXITSTATUS(status) == 127) {
                    fprintf(stderr, "execv failed\n");
                }
            } else if (WIFSIGNALED(status))  {
                fprintf(stderr, "signal received: %d\n", WIFSIGNALED(status));
            }
        } else {
            fprintf(stderr, "waitpid() failed\n");
        }
    }

    if (!access(t_name, F_OK)) {
        remove(t_name);
    }

    proc->returncode = status;
    guard_free(t_name);
    return WEXITSTATUS(status);
}

int shell_safe(struct Process *proc, char *args) {
    FILE *fp;
    char buf[1024] = {0};

    char *invalid_ch = strpbrk(args, STASIS_SHELL_SAFE_RESTRICT);
    if (invalid_ch) {
        args = NULL;
    }

    int result = shell(proc, args);
    if (strlen(proc->f_stdout)) {
        fp = fopen(proc->f_stdout, "r");
        if (fp) {
            while (fgets(buf, sizeof(buf) - 1, fp)) {
                fprintf(stdout, "%s", buf);
                buf[0] = '\0';
            }
            fclose(fp);
            fp = NULL;
        }
    }
    if (strlen(proc->f_stderr)) {
        fp = fopen(proc->f_stderr, "r");
        if (fp) {
            while (fgets(buf, sizeof(buf) - 1, fp)) {
                fprintf(stderr, "%s", buf);
                buf[0] = '\0';
            }
            fclose(fp);
            fp = NULL;
        }
    }
    return result;
}

char *shell_output(const char *command, int *status) {
    const size_t initial_size = STASIS_BUFSIZ;
    size_t current_size = initial_size;
    char *result = NULL;
    char line[STASIS_BUFSIZ];

    errno = 0;
    *status = 0;
    FILE *pp = popen(command, "r");
    if (!pp) {
        *status = -1;
        return NULL;
    }

    if (errno) {
        *status = 1;
    }
    result = calloc(initial_size, sizeof(result));
    memset(line, 0, sizeof(line));
    while (fread(line, sizeof(char), sizeof(line) - 1, pp) != 0) {
        size_t result_len = strlen(result);
        size_t need_realloc = (result_len + strlen(line)) > current_size;
        if (need_realloc) {
            current_size += initial_size;
            char *tmp = realloc(result, sizeof(*result) * current_size);
            if (!tmp) {
                return NULL;
            } else if (tmp != result) {
                result = tmp;
            }
        }
        strcat(result, line);
        memset(line, 0, sizeof(line));
    }
    *status = pclose(pp);
    return result;
}
