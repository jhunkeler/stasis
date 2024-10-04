//! @file conda.h
#ifndef STASIS_CONDA_H
#define STASIS_CONDA_H

#include <stdio.h>
#include <string.h>
#include "core.h"

#define CONDA_INSTALL_PREFIX "conda"
#define PYPI_INDEX_DEFAULT "https://pypi.org/simple"

struct MicromambaInfo {
    char *micromamba_prefix;    //!< Path to write micromamba binary
    char *conda_prefix;         //!< Path to install conda base tree
};

/**
 * Execute micromamba
 * @param info MicromambaInfo data structure (must be populated before use)
 * @param command printf-style formatter string
 * @param ... variadic arguments
 * @return exit code
 */
int micromamba(struct MicromambaInfo *info, char *command, ...);

/**
 * Execute Python
 * Python interpreter is determined by PATH
 *
 * ```c
 * if (python_exec("-c 'printf(\"Hello world\")'")) {
 *     fprintf(stderr, "Hello world failed\n");
 *     exit(1);
 * }
 * ```
 *
 * @param args arguments to pass to interpreter
 * @return exit code from python interpreter
 */
int python_exec(const char *args);

/**
 * Execute Pip
 * Pip is determined by PATH
 *
 * ```c
 * if (pip_exec("freeze")) {
 *     fprintf(stderr, "pip freeze failed\n");
 *     exit(1);
 * }
 * ```
 *
 * @param args arguments to pass to Pip
 * @return exit code from Pip
 */
int pip_exec(const char *args);

/**
 * Execute conda (or if possible, mamba)
 * Conda/Mamba is determined by PATH
 *
 * ```c
 * if (conda_exec("env list")) {
 *     fprintf(stderr, "Failed to list conda environments\n");
 *     exit(1);
 * }
 * ```
 *
 * @param args arguments to pass to Conda
 * @return exit code from Conda
 */
int conda_exec(const char *args);

/**
 * Configure the runtime environment to use Conda/Mamba
 *
 * ```c
 * if (conda_activate("/path/to/conda/installation", "base")) {
 *     fprintf(stderr, "Failed to activate conda's base environment\n");
 *     exit(1);
 * }
 * ```
 *
 * @param root directory where conda is installed
 * @param env_name the conda environment to activate
 * @return 0 on success, -1 on error
 */
int conda_activate(const char *root, const char *env_name);

/**
 * Configure the active conda installation for headless operation
 */
int conda_setup_headless();

/**
 * Creates a Conda environment from a YAML config
 *
 * ```c
 * if (conda_env_create_from_uri("myenv", "https://myserver.tld/environment.yml")) {
 *     fprintf(stderr, "Environment creation failed\n");
 *     exit(1);
 * }
 * ```
 *
 * @param name Name of new environment to create
 * @param uri /path/to/environment.yml
 * @param uri file:///path/to/environment.yml
 * @param uri http://myserver.tld/environment.yml
 * @param uri https://myserver.tld/environment.yml
 * @param uri ftp://myserver.tld/environment.yml
 * @return exit code from "conda"
 */
int conda_env_create_from_uri(char *name, char *uri);

/**
 * Create a Conda environment using generic package specs
 *
 * ```c
 * // Create a basic environment without any conda packages
 * if (conda_env_create("myenv", "3.11", NULL)) {
 *     fprintf(stderr, "Environment creation failed\n");
 *     exit(1);
 * }
 *
 * // Create a basic environment and install conda packages
 * if (conda_env_create("myenv", "3.11", "hstcal fitsverify")) {
 *     fprintf(stderr, "Environment creation failed\n");
 *     exit(1);
 * }
 * ```
 *
 * @param name Environment name
 * @param python_version Desired version of Python
 * @param packages Packages to install (or NULL)
 * @return exit code from "conda"
 */
int conda_env_create(char *name, char *python_version, char *packages);

/**
 * Remove a Conda environment
 *
 * ```c
 * if (conda_env_remove("myenv")) {
 *     fprintf(stderr, "Unable to remove conda environment\n");
 *     exit(1);
 * }
 * ```
 *
 * @param name Environment name
 * @return exit code from "conda"
 */
int conda_env_remove(char *name);

/**
 * Export a Conda environment in YAML format
 *
 * ```c
 * if (conda_env_export("myenv", "./", "myenv.yml")) {
 *     fprintf(stderr, "Unable to export environment\n");
 *     exit(1);
 * }
 * ```
 *
 * @param name Environment name to export
 * @param output_dir Destination directory
 * @param output_filename Destination file name
 * @return exit code from "conda"
 */
int conda_env_export(char *name, char *output_dir, char *output_filename);

/**
 * Run "conda index" on a local conda channel
 *
 * ```c
 * if (conda_index("/path/to/channel")) {
 *     fprintf(stderr, "Unable to index requested path\n");
 *     exit(1);
 * }
 * ```
 *
 * @param path Top-level directory of conda channel
 * @return exit code from "conda"
 */
int conda_index(const char *path);

/**
 * Determine whether a simple index contains a package
 * @param index_url a file system path or url pointing to a simple index
 * @param name package name (required)
 * @param version package version (may be NULL)
 * @return not found = 0, found = 1, error = -1
 */
int pip_index_provides(const char *index_url, const char *name, const char *version);

char *conda_get_active_environment();

int conda_provides(const char *spec);


#endif //STASIS_CONDA_H
