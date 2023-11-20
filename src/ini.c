#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "omc.h"
#include "ini.h"

struct INIFILE *ini_init() {
    struct INIFILE *ini;
    ini = calloc(1, sizeof(*ini));
    ini->section_count = 0;
    return ini;
}

void ini_section_init(struct INIFILE **ini) {
    (*ini)->section = calloc((*ini)->section_count + 1, sizeof(**(*ini)->section));
}

struct INISection *ini_section_search(struct INIFILE **ini, char *value) {
    struct INISection *result = NULL;
    for (size_t i = 0; i < (*ini)->section_count; i++) {
        if ((*ini)->section[i]->key != NULL) {
            if (!strcmp((*ini)->section[i]->key, value)) {
                result = (*ini)->section[i];
            }
        }
    }
    return result;
}

int ini_data_init(struct INIFILE **ini, char *section_name) {
    struct INISection *section = ini_section_search(ini, section_name);
    if (section == NULL) {
        return 1;
    }
    section->data = calloc(section->data_count + 1, sizeof(**section->data));
    return 0;
}

struct INIData *ini_data_get(struct INIFILE *ini, char *section_name, char *key) {
    struct INISection *section = NULL;
    section = ini_section_search(&ini, section_name);
    for (size_t i = 0; i < section->data_count; i++) {
        if (section->data[i]->key != NULL) {
            if (!strcmp(section->data[i]->key, key)) {
                return section->data[i];
            }
        }
    }
    return NULL;
}

struct INIData *ini_getall(struct INIFILE *ini, char *section_name) {
    struct INISection *section = NULL;
    struct INIData *result = NULL;
    static size_t i = 0;

    section = ini_section_search(&ini, section_name);
    if (section->data[i]) {
        result = section->data[i];
        i++;
    } else {
        result = NULL;
        i = 0;
    }

    return result;
}

int ini_getval(struct INIFILE *ini, char *section_name, char *key, int type, union INIVal *result) {
    char *token = NULL;
    char tbuf[OMC_BUFSIZ];
    char *tbufp = tbuf;
    struct INIData *data;
    data = ini_data_get(ini, section_name, key);
    if (!data) {
        result->as_char_p = NULL;
        return -1;
    }
    switch (type) {
        case INIVAL_TYPE_INT:
            result->as_int = (int) strtol(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_UINT:
            result->as_uint = (unsigned int) strtoul(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_LONG:
            result->as_long = (long) strtol(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_ULONG:
            result->as_ulong = (unsigned long) strtoul(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_LLONG:
            result->as_llong = (long long) strtoll(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_ULLONG:
            result->as_ullong = (unsigned long long) strtoull(data->value, NULL, 10);
            break;
        case INIVAL_TYPE_DOUBLE:
            result->as_double = (double) strtod(data->value, NULL);
            break;
        case INIVAL_TYPE_FLOAT:
            result->as_float = (float) strtod(data->value, NULL);
            break;
        case INIVAL_TYPE_STR:
            result->as_char_p = lstrip(data->value);
            break;
        case INIVAL_TYPE_STR_ARRAY:
            strcpy(tbufp, data->value);
            *data->value = '\0';
            for (size_t i = 0; (token = strsep(&tbufp, "\n")) != NULL; i++) {
                lstrip(token);
                strcat(data->value, token);
                strcat(data->value, "\n");
            }
            result->as_char_p = data->value;
            break;
        case INIVAL_TYPE_BOOL:
            result->as_bool = false;
            if ((!strcmp(data->value, "true") || !strcmp(data->value, "True")) ||
                    (!strcmp(data->value, "yes") || !strcmp(data->value, "Yes")) ||
                    strtol(data->value, NULL, 10)) {
                result->as_bool = true;
            }
            break;
        default:
            memset(result, 0, sizeof(*result));
            break;
    }
    return 0;
}

int ini_data_record(struct INIFILE **ini, char *section_name, char *key, char *value) {
    struct INISection *section = ini_section_search(ini, section_name);
    if (section == NULL) {
        return 1;
    }

    struct INIData **tmp = realloc(section->data, (section->data_count + 1) * sizeof(**section->data));
    if (!tmp) {
        perror(__FUNCTION__);
        exit(1);
    }
    section->data = tmp;
    if (!ini_data_get((*ini), section_name, key)) {
        section->data[section->data_count] = calloc(1, sizeof(*section->data[0]));
        section->data[section->data_count]->key = key; //strdup(key);
        section->data[section->data_count]->value = value; //strdup(value);
        section->data_count++;
    } else {
        struct INIData *data = ini_data_get(*ini, section_name, key);
        size_t value_len_old = strlen(data->value);
        size_t value_len = strlen(value);
        size_t value_len_new = value_len_old + value_len;
        /*
        char *value_tmp = NULL;
        value_tmp = realloc(data->value, value_len_new + 2);
        if (!value_tmp) {
            perror(__FUNCTION__ );
            exit(1);
        }
        data->value = value_tmp;
         */
        //strcat(data->value, " ");
        strcat(data->value, value);
    }
    return 0;
}

void ini_section_record(struct INIFILE **ini, char *key) {
    struct INISection **tmp = realloc((*ini)->section, ((*ini)->section_count + 1) * sizeof((*ini)->section));
    if (!tmp) {
        perror(__FUNCTION__);
        exit(1);
    }
    (*ini)->section = tmp;
    (*ini)->section[(*ini)->section_count] = calloc(1, sizeof(*(*ini)->section[0]));
    (*ini)->section[(*ini)->section_count]->key = strdup(key);
    (*ini)->section_count++;
}

void ini_show(struct INIFILE *ini) {
    for (size_t x = 0; x < ini->section_count; x++) {
        printf("[%s]\n", ini->section[x]->key);
        for (size_t y = 0; y < ini->section[x]->data_count; y++) {
            printf("%s='%s'\n", ini->section[x]->data[y]->key, ini->section[x]->data[y]->value);
        }
        printf("\n");
    }
}

char *unquote(char *s) {
    int found = 0;
    if (startswith(s, "'") && endswith(s, "'")) {
        found = 1;
    } else if (startswith(s, "\"") && endswith(s, "\"")) {
        found = 1;
    }

    if (found) {
        memmove(s, s + 1, strlen(s));
        s[strlen(s) - 1] = '\0';
    }
    return s;
}

char *collapse_whitespace(char **s) {
    size_t len = strlen(*s);
    size_t i;
    for (i = 0; isblank((int)*s[i]); i++);
    memmove(*s, *s + i, strlen(*s));
    if (i) {
        *s[len - i] = '\0';
    }
    return *s;
}

void ini_free(struct INIFILE **ini) {
    for (size_t section = 0; section < (*ini)->section_count; section++) {
        for (size_t data = 0; data < (*ini)->section[section]->data_count; data++) {
            if ((*ini)->section[section]->data[data]) {
                free((*ini)->section[section]->data[data]->key);
                free((*ini)->section[section]->data[data]->value);
                free((*ini)->section[section]->data[data]);
            }
        }
        free((*ini)->section[section]->data);
        free((*ini)->section[section]->key);
        free((*ini)->section[section]);
    }
    free((*ini)->section);
    free((*ini));
}

struct INIFILE *ini_open(const char *filename) {
    FILE *fp;
    char line[OMC_BUFSIZ] = {0};
    char current_section[OMC_BUFSIZ] = {0};
    char *key_last = NULL;
    struct INIFILE *ini = ini_init();

    ini_section_init(&ini);

    // Create an implicit section. [default] does not need to be present in the INI config
    ini_section_record(&ini, "default");
    strcpy(current_section, "default");
    //ini_data_init(&ini, "default");

    // Open the configuration file for reading
    fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        exit(1);
    }

    // Read file
    for (size_t i = 0; fgets(line, sizeof(line), fp) != NULL; i++) {
        // Find pointer to first comment character
        char *comment = strpbrk(line, ";#");
        if (comment) {
            // Remove comment from line (standalone and inline comments)
            if (!(comment - line > 0 && (*(comment - 1) == '\\') || (*comment - 1) == '#')) {
                *comment = '\0';
            } else {
                // Handle escaped comment characters. Remove the escape character '\'
                memmove(comment - 1, comment, strlen(comment));
                comment[strlen(comment) - 1] = '\0';
            }
        }

        // Removing comments could have reduced the line's length, so calculate it now
        size_t len = strlen(line);

        // Ignore empty lines
        if (!len || line[0] == '\n') {
            continue;
        }

        // Test for section header: [string]
        if (startswith(line, "[")) {
            // Ignore default section because we already have an implicit one
            if (!strncmp(&line[1], "default", strlen("default"))) {
                continue;
            }

            // Remove section ending: ']'
            line[strlen(line) - 2] = '\0';

            // Create new named section
            ini_section_record(&ini, &line[1]);
            //ini_data_init(&ini, &line[1]);

            // Record the name of the section. This is used until another section is found.
            strcpy(current_section, &line[1]);
            continue;
        }

        char *key = NULL;
        char *value = malloc(OMC_BUFSIZ);
        char *operator = strchr(line, '=');

        // continuation line
        if (startswith(line, " ") || startswith(line, "\t")) {
            operator = NULL;
        }

        if (operator) {
            size_t key_len = operator - line;
            key = strndup(line, key_len);
            key_last = key;
            strcpy(value, &operator[1]);
            if (strlen(value)) {
                value[strlen(value) - 1] = '\0';
            }
        } else if (!key && !strlen(value) && ! (startswith(line, " ") || startswith(line, "\t"))) {
            fprintf(stderr, "NO OPERATOR OR INDENT: %zu:'%s'\n", i, line);
            struct INISection *section = ini_section_search(&ini, current_section);
            struct INIData *data = NULL;
            //key = key_last;
            free(value);
            value = NULL;
        } else {
            struct INISection *section = ini_section_search(&ini, current_section);
            struct INIData *data = section->data[section->data_count - 1];
            if (strlen(data->value)) {
                data->value[strlen(data->value) - 1] = '\n';
            }
            key = key_last;
            strcpy(value, line);
            if (endswith(value, "\n")) {
                if (strlen(value)) {
                    value[strlen(value) - 1] = '\n';
                }
            }
        }

        // Store key value pair in section's data array
        if (key) {
            lstrip(key);
            strip(key);
            unquote(value);
            lstrip(value);
            ini_data_record(&ini, current_section, key, value);
        }
    }

    return ini;
}