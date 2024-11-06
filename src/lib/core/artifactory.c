#include "artifactory.h"

int artifactory_download_cli(char *dest,
                             char *jfrog_artifactory_base_url,
                             char *jfrog_artifactory_product,
                             char *cli_major_ver,
                             char *version,
                             char *os,
                             char *arch,
                             char *remote_filename) {
    char url[PATH_MAX] = {0};
    char path[PATH_MAX] = {0};
    char os_ident[STASIS_NAME_MAX] = {0};
    char arch_ident[STASIS_NAME_MAX] = {0};

    // convert platform string to lower-case
    strcpy(os_ident, os);
    tolower_s(os_ident);

    // translate OS identifier
    if (!strcmp(os_ident, "darwin") || startswith(os_ident, "macos")) {
        strcpy(os_ident, "mac");
    } else if (!strcmp(os_ident, "linux")) {
        strcpy(os_ident, "linux");
    } else {
        fprintf(stderr, "%s: unknown operating system: %s\n", __FUNCTION__, os_ident);
        return -1;
    }

    // translate ARCH identifier
    strcpy(arch_ident, arch);
    if (startswith(arch_ident, "i") && endswith(arch_ident, "86")) {
        strcpy(arch_ident, "386");
    } else if (!strcmp(arch_ident, "amd64") || !strcmp(arch_ident, "x86_64") || !strcmp(arch_ident, "x64")) {
        if (!strcmp(os_ident, "mac")) {
            strcpy(arch_ident, "386");
        } else {
            strcpy(arch_ident, "amd64");
        }
    } else if (!strcmp(arch_ident, "arm64") || !strcmp(arch_ident, "aarch64")) {
        strcpy(arch_ident, "arm64");
    } else {
        fprintf(stderr, "%s: unknown architecture: %s\n", __FUNCTION__, arch_ident);
        return -1;
    }

    snprintf(url, sizeof(url) - 1, "%s/%s/%s/%s/%s-%s-%s/%s",
             jfrog_artifactory_base_url,           // https://releases.jfrog.io/artifactory
             jfrog_artifactory_product,            // jfrog-cli
             cli_major_ver,                        // v\d+(-jf)?
             version,                              // 1.2.3
             jfrog_artifactory_product,            // ...
             os_ident,                             // ...
             arch_ident,                           // jfrog-cli-linux-x86_64
             remote_filename);                     // jf
    strcpy(path, dest);

    if (mkdirs(path, 0755)) {
        fprintf(stderr, "%s: %s: %s", __FUNCTION__, path, strerror(errno));
        return -1;
    }

    sprintf(path + strlen(path), "/%s", remote_filename);
    long fetch_status = download(url, path, NULL);
    if (HTTP_ERROR(fetch_status) || fetch_status < 0) {
        fprintf(stderr, "%s: download failed: %s\n", __FUNCTION__, url);
        return -1;
    }
    chmod(path, 0755);
    return 0;
}

void jfrt_register_opt_str(char *jfrt_val, const char *opt_name, struct StrList **opt_map) {
    char data[STASIS_BUFSIZ] = {0};

    if (jfrt_val == NULL) {
        // no data
        return;
    }
    snprintf(data, sizeof(data) - 1, "--%s=\"%s\"", opt_name, jfrt_val);
    strlist_append(&*opt_map, data);
}

void jfrt_register_opt_bool(bool jfrt_val, const char *opt_name, struct StrList **opt_map) {
    char data[STASIS_BUFSIZ] = {0};

    if (jfrt_val == false) {
        // option will not be used
        return;
    }
    snprintf(data, sizeof(data) - 1, "--%s", opt_name);
    strlist_append(&*opt_map, data);
}

void jfrt_register_opt_int(int jfrt_val, const char *opt_name, struct StrList **opt_map) {
    char data[STASIS_BUFSIZ] = {0};

    if (jfrt_val == 0) {
        // option will not be used
        return;
    }
    snprintf(data, sizeof(data) - 1, "--%s=%d", opt_name, jfrt_val);
    strlist_append(&*opt_map, data);
}

void jfrt_register_opt_long(long jfrt_val, const char *opt_name, struct StrList **opt_map) {
    char data[STASIS_BUFSIZ] = {0};

    if (jfrt_val == 0) {
        // option will not be used
        return;
    }
    snprintf(data, sizeof(data) - 1, "--%s=%ld", opt_name, jfrt_val);
    strlist_append(&*opt_map, data);
}

void jfrt_upload_init(struct JFRT_Upload *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->recursive = true;
    ctx->threads = 3;
    ctx->retries = 3;
}

static int auth_required(const char *cmd) {
    const char *modes[] = {
            "build-collect-env",
            NULL,
    };
    for (size_t i = 0; modes[i] != NULL; i++) {
        if (!startswith(cmd, modes[i])) {
            return 1;
        }
    }
    return 0;
}

int jfrt_auth_init(struct JFRT_Auth *auth_ctx) {
    char *url = getenv("STASIS_JF_ARTIFACTORY_URL");
    char *user = getenv("STASIS_JF_USER");
    char *access_token = getenv("STASIS_JF_ACCESS_TOKEN");
    char *password = getenv("STASIS_JF_PASSWORD");
    char *ssh_key_path = getenv("STASIS_JF_SSH_KEY_PATH");
    char *ssh_passphrase = getenv("STASIS_JF_SSH_PASSPHRASE");
    char *client_cert_key_path = getenv("STASIS_JF_CLIENT_CERT_KEY_PATH");
    char *client_cert_path = getenv("STASIS_JF_CLIENT_CERT_PATH");

    if (!url) {
        fprintf(stderr, "Artifactory URL is not configured:\n");
        fprintf(stderr, "please set STASIS_JF_ARTIFACTORY_URL\n");
        return -1;
    }
    auth_ctx->url = url;

    if (access_token) {
        auth_ctx->user = NULL;
        auth_ctx->access_token = access_token;
        auth_ctx->password = NULL;
        auth_ctx->ssh_key_path = NULL;
    } else if (user && password) {
        auth_ctx->user = user;
        auth_ctx->password = password;
        auth_ctx->access_token = NULL;
        auth_ctx->ssh_key_path = NULL;
    } else if (ssh_key_path) {
        auth_ctx->user = NULL;
        auth_ctx->ssh_key_path = ssh_key_path;
        if (ssh_passphrase) {
            auth_ctx->ssh_passphrase = ssh_passphrase;
        }
        auth_ctx->password = NULL;
        auth_ctx->access_token = NULL;
    } else if (client_cert_key_path && client_cert_path) {
        auth_ctx->user = NULL;
        auth_ctx->password = NULL;
        auth_ctx->access_token = NULL;
        auth_ctx->ssh_key_path = NULL;
        auth_ctx->client_cert_key_path = client_cert_key_path;
        auth_ctx->client_cert_path = client_cert_path;
    } else {
        fprintf(stderr, "Artifactory authentication is not configured:\n");
        fprintf(stderr, "set STASIS_JF_USER and STASIS_JF_PASSWORD\n");
        fprintf(stderr, "or, set STASIS_JF_ACCESS_TOKEN\n");
        fprintf(stderr, "or, set STASIS_JF_SSH_KEY_PATH and STASIS_JF_SSH_KEY_PASSPHRASE\n");
        fprintf(stderr, "or, set STASIS_JF_CLIENT_CERT_KEY_PATH and STASIS_JF_CLIENT_CERT_PATH\n");
        return -1;
    }
    return 0;
}

int jfrog_cli(struct JFRT_Auth *auth, const char *subsystem, const char *task, char *args) {
    struct Process proc;
    char cmd[STASIS_BUFSIZ];
    char cmd_redacted[STASIS_BUFSIZ];

    memset(&proc, 0, sizeof(proc));
    memset(cmd, 0, sizeof(cmd));
    memset(cmd_redacted, 0, sizeof(cmd_redacted));

    struct StrList *arg_map = strlist_init();
    if (!arg_map) {
        return -1;
    }

    char *auth_args = NULL;
    if (auth_required(task)) {
        // String options
        jfrt_register_opt_str(auth->url, "url", &arg_map);
        jfrt_register_opt_str(auth->user, "user", &arg_map);
        jfrt_register_opt_str(auth->access_token, "access-token", &arg_map);
        jfrt_register_opt_str(auth->password, "password", &arg_map);
        jfrt_register_opt_str(auth->ssh_key_path, "ssh-key-path", &arg_map);
        jfrt_register_opt_str(auth->ssh_passphrase, "ssh-passphrase", &arg_map);
        jfrt_register_opt_str(auth->client_cert_key_path, "client-cert-key-path", &arg_map);
        jfrt_register_opt_str(auth->client_cert_path, "client-cert-path", &arg_map);
        jfrt_register_opt_bool(auth->insecure_tls, "insecure-tls", &arg_map);
        jfrt_register_opt_str(auth->server_id, "server-id", &arg_map);
    }

    auth_args = join(arg_map->data, " ");
    if (!auth_args) {
        return -1;
    }

    const char *redactable[] = {
            auth->access_token,
            auth->ssh_key_path,
            auth->ssh_passphrase,
            auth->client_cert_key_path,
            auth->client_cert_path,
            auth->password,
    };
    snprintf(cmd, sizeof(cmd) - 1, "jf %s %s %s %s", subsystem, task, auth_args, args ? args : "");
    redact_sensitive(redactable, sizeof(redactable) / sizeof (*redactable), cmd, cmd_redacted, sizeof(cmd_redacted) - 1);

    guard_free(auth_args);
    guard_strlist_free(&arg_map);

    // Pings are noisy. Squelch them.
    if (task && !strstr(task, "ping")) {
        msg(STASIS_MSG_L2, "Executing: %s\n", cmd_redacted);
    }

    if (!globals.verbose) {
        strcpy(proc.f_stdout, "/dev/null");
        strcpy(proc.f_stderr, "/dev/null");
    }
    return shell(&proc, cmd);
}

static int jfrog_cli_rt(struct JFRT_Auth *auth, char *task, char *args) {
    return jfrog_cli(auth, "rt", task, args);
}

int jfrog_cli_rt_build_collect_env(struct JFRT_Auth *auth, char *build_name, char *build_number) {
    char cmd[STASIS_BUFSIZ] = {0};
    snprintf(cmd, sizeof(cmd) - 1, "\"%s\" \"%s\"", build_name, build_number);
    return jfrog_cli(auth, "rt", "build-collect-env", cmd);
}

int jfrog_cli_rt_build_publish(struct JFRT_Auth *auth, char *build_name, char *build_number) {
    char cmd[STASIS_BUFSIZ] = {0};
    snprintf(cmd, sizeof(cmd) - 1, "\"%s\" \"%s\"", build_name, build_number);
    return jfrog_cli(auth, "rt", "build-publish", cmd);
}

int jfrog_cli_rt_ping(struct JFRT_Auth *auth) {
    return jfrog_cli_rt(auth, "ping", NULL);
}

int jfrog_cli_rt_download(struct JFRT_Auth *auth, struct JFRT_Download *ctx, char *repo_path, char *dest) {
    char cmd[STASIS_BUFSIZ] = {0};

    if (isempty(repo_path)) {
        fprintf(stderr, "repo_path argument must be a valid artifactory repository path\n");
        return -1;
    }

    // dest is an optional argument, therefore may be NULL or an empty string

    struct StrList *arg_map = strlist_init();
    if (!arg_map) {
        return -1;
    }

    jfrt_register_opt_str(ctx->archive_entries, "archive-entries", &arg_map);
    jfrt_register_opt_str(ctx->build, "build", &arg_map);
    jfrt_register_opt_str(ctx->build_name, "build-name", &arg_map);
    jfrt_register_opt_str(ctx->build_number, "build-number", &arg_map);
    jfrt_register_opt_str(ctx->bundle, "bundle", &arg_map);
    jfrt_register_opt_str(ctx->exclude_artifacts, "exclude-artifacts", &arg_map);
    jfrt_register_opt_str(ctx->exclude_props, "exclude-props", &arg_map);
    jfrt_register_opt_str(ctx->exclusions, "exclusions", &arg_map);
    jfrt_register_opt_str(ctx->gpg_key, "gpg-key", &arg_map);
    jfrt_register_opt_str(ctx->include_deps, "include-deps", &arg_map);
    jfrt_register_opt_str(ctx->include_dirs, "include-dirs", &arg_map);
    jfrt_register_opt_str(ctx->module, "module", &arg_map);
    jfrt_register_opt_str(ctx->project, "project", &arg_map);
    jfrt_register_opt_str(ctx->props, "props", &arg_map);
    jfrt_register_opt_str(ctx->sort_by, "sort-by", &arg_map);
    jfrt_register_opt_str(ctx->sort_order, "sort-order", &arg_map);
    jfrt_register_opt_str(ctx->spec, "spec", &arg_map);
    jfrt_register_opt_str(ctx->spec_vars, "spec-vars", &arg_map);

    jfrt_register_opt_bool(ctx->detailed_summary, "detailed-summary", &arg_map);
    jfrt_register_opt_bool(ctx->dry_run, "dry-run", &arg_map);
    jfrt_register_opt_bool(ctx->explode, "explode", &arg_map);
    jfrt_register_opt_bool(ctx->fail_no_op, "fail-no-op", &arg_map);
    jfrt_register_opt_bool(ctx->flat, "flat", &arg_map);
    jfrt_register_opt_bool(ctx->quiet, "quiet", &arg_map);
    jfrt_register_opt_bool(ctx->recursive, "recursive", &arg_map);
    jfrt_register_opt_bool(ctx->retries, "retries", &arg_map);
    jfrt_register_opt_bool(ctx->retry_wait_time, "retry-wait-time", &arg_map);
    jfrt_register_opt_bool(ctx->skip_checksum, "skip-checksum", &arg_map);

    jfrt_register_opt_int(ctx->limit, "limit", &arg_map);
    jfrt_register_opt_int(ctx->min_split, "min-split", &arg_map);
    jfrt_register_opt_int(ctx->offset, "offset", &arg_map);
    jfrt_register_opt_int(ctx->split_count, "split-count", &arg_map);
    jfrt_register_opt_int(ctx->sync_deletes, "sync-deletes", &arg_map);
    jfrt_register_opt_int(ctx->threads, "threads", &arg_map);
    jfrt_register_opt_int(ctx->validate_symlinks, "validate-symlinks", &arg_map);

    char *args = join(arg_map->data, " ");
    if (!args) {
        return -1;
    }

    snprintf(cmd, sizeof(cmd) - 1, "%s '%s' %s", args, repo_path, dest ? dest : "");
    guard_free(args);
    guard_strlist_free(&arg_map);

    int status = jfrog_cli_rt(auth, "download", cmd);
    return status;
}

int jfrog_cli_rt_upload(struct JFRT_Auth *auth, struct JFRT_Upload *ctx, char *src, char *repo_path) {
    char cmd[STASIS_BUFSIZ] = {0};

    if (isempty(src)) {
        fprintf(stderr, "src argument must be a valid file system path\n");
        return -1;
    }

    if (isempty(repo_path)) {
        fprintf(stderr, "repo_path argument must be a valid artifactory repository path\n");
        return -1;
    }

    struct StrList *arg_map = strlist_init();
    if (!arg_map) {
        return -1;
    }

    // String options
    jfrt_register_opt_str(ctx->build_name, "build-name", &arg_map);
    jfrt_register_opt_str(ctx->build_number, "build-number", &arg_map);
    jfrt_register_opt_str(ctx->exclusions, "exclusions", &arg_map);
    jfrt_register_opt_str(ctx->module, "module", &arg_map);
    jfrt_register_opt_str(ctx->spec, "spec", &arg_map);
    jfrt_register_opt_str(ctx->spec_vars, "spec-vars", &arg_map);
    jfrt_register_opt_str(ctx->project, "project", &arg_map);
    jfrt_register_opt_str(ctx->target_props, "target-props", &arg_map);

    // Boolean options
    jfrt_register_opt_bool(ctx->quiet, "quiet", &arg_map);
    jfrt_register_opt_bool(ctx->ant, "ant", &arg_map);
    jfrt_register_opt_bool(ctx->archive, "archive", &arg_map);
    jfrt_register_opt_bool(ctx->deb, "deb", &arg_map);
    jfrt_register_opt_bool(ctx->detailed_summary, "detailed-summary", &arg_map);
    jfrt_register_opt_bool(ctx->dry_run, "dry-run", &arg_map);
    jfrt_register_opt_bool(ctx->explode, "explode", &arg_map);
    jfrt_register_opt_bool(ctx->fail_no_op, "fail-no-op", &arg_map);
    jfrt_register_opt_bool(ctx->flat, "flat", &arg_map);
    jfrt_register_opt_bool(ctx->include_dirs, "include-dirs", &arg_map);
    jfrt_register_opt_bool(ctx->recursive, "recursive", &arg_map);
    jfrt_register_opt_bool(ctx->symlinks, "symlinks", &arg_map);
    jfrt_register_opt_bool(ctx->sync_deletes, "sync-deletes", &arg_map);
    jfrt_register_opt_bool(ctx->regexp, "regexp", &arg_map);

    // Integer options
    jfrt_register_opt_int(ctx->retries, "retries", &arg_map);
    jfrt_register_opt_int(ctx->retry_wait_time, "retry-wait-time", &arg_map);
    jfrt_register_opt_int(ctx->threads, "threads", &arg_map);

    char *args = join(arg_map->data, " ");
    if (!args) {
        return -1;
    }

    char *new_src = NULL;
    char *base = NULL;
    if (ctx->workaround_parent_only) {
        struct StrList *components = strlist_init();

        strlist_append_tokenize(components, src, "/");
        int max_components = (int) strlist_count(components);
        for (int i = 0; i < max_components; i++) {
            if (strstr(components->data[i], "*")) {
                max_components = i;
                break;
            }
        }
        base = join(&components->data[max_components], "/");
        guard_free(components->data[max_components]);
        new_src = join(components->data, "/");
        guard_strlist_free(&components);
    }

    if (new_src) {
        if (base) {
            src = base;
        } else {
            strcat(src, "/");
        }
        pushd(new_src);
    }

    snprintf(cmd, sizeof(cmd) - 1, "%s '%s' \"%s\"", args, src, repo_path);
    guard_free(args);
    guard_strlist_free(&arg_map);

    int status = jfrog_cli_rt(auth, "upload", cmd);
    if (new_src) {
        popd();
        guard_free(new_src);
    }
    if (base) {
        guard_free(base);
    }

    return status;
}

int jfrog_cli_rt_search(struct JFRT_Auth *auth, struct JFRT_Search *ctx, char *repo_path, char *pattern) {
    char cmd[STASIS_BUFSIZ] = {0};

    if (isempty(repo_path)) {
        fprintf(stderr, "repo_path argument must be a valid artifactory repository path\n");
        return -1;
    }

    struct StrList *arg_map = strlist_init();
    if (!arg_map) {
        return -1;
    }

    jfrt_register_opt_str(ctx->archive_entries, "archive-entries", &arg_map);
    jfrt_register_opt_str(ctx->build, "build", &arg_map);
    jfrt_register_opt_str(ctx->bundle, "bundle", &arg_map);
    jfrt_register_opt_str(ctx->exclusions, "exclusions", &arg_map);
    jfrt_register_opt_str(ctx->exclude_patterns, "exclude-patterns", &arg_map);
    jfrt_register_opt_str(ctx->exclude_artifacts, "exclude-artifacts", &arg_map);
    jfrt_register_opt_str(ctx->exclude_props, "exclude-props", &arg_map);
    jfrt_register_opt_str(ctx->include, "include", &arg_map);
    jfrt_register_opt_str(ctx->include_deps, "include_deps", &arg_map);
    jfrt_register_opt_str(ctx->include_dirs, "include_dirs", &arg_map);
    jfrt_register_opt_str(ctx->project, "project", &arg_map);
    jfrt_register_opt_str(ctx->props, "props", &arg_map);
    jfrt_register_opt_str(ctx->sort_by, "sort-by", &arg_map);
    jfrt_register_opt_str(ctx->sort_order, "sort-order", &arg_map);
    jfrt_register_opt_str(ctx->spec, "spec", &arg_map);
    jfrt_register_opt_str(ctx->spec_vars, "spec-vars", &arg_map);

    jfrt_register_opt_bool(ctx->count, "count", &arg_map);
    jfrt_register_opt_bool(ctx->fail_no_op, "fail-no-op", &arg_map);
    jfrt_register_opt_bool(ctx->recursive, "recursive", &arg_map);
    jfrt_register_opt_bool(ctx->transitive, "transitive", &arg_map);

    jfrt_register_opt_int(ctx->limit, "limit", &arg_map);
    jfrt_register_opt_int(ctx->offset, "offset", &arg_map);

    char *args = join(arg_map->data, " ");
    if (!args) {
        return -1;
    }

    snprintf(cmd, sizeof(cmd) - 1, "%s '%s/%s'", args, repo_path, pattern ? pattern: "");
    guard_free(args);
    guard_strlist_free(&arg_map);

    int status = jfrog_cli_rt(auth, "search", cmd);
    return status;
}
