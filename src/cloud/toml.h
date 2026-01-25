// Zen-C Cloud Build - TOML Parser
// Lightweight TOML parser for zen.toml configuration

#ifndef ZC_TOML_H
#define ZC_TOML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// TOML Value Types
typedef enum {
    TOML_NONE = 0,
    TOML_STRING,
    TOML_INT,
    TOML_BOOL,
    TOML_ARRAY,
    TOML_TABLE
} TomlType;

// Forward declarations
typedef struct TomlValue TomlValue;
typedef struct TomlArray TomlArray;
typedef struct TomlTable TomlTable;
typedef struct TomlEntry TomlEntry;

// TOML Array
struct TomlArray {
    TomlValue **items;
    int count;
    int capacity;
};

// TOML Table Entry
struct TomlEntry {
    char *key;
    TomlValue *value;
    struct TomlEntry *next;
};

// TOML Table
struct TomlTable {
    TomlEntry *entries;
    int count;
};

// TOML Value
struct TomlValue {
    TomlType type;
    union {
        char *string_val;
        long long int_val;
        int bool_val;
        TomlArray *array_val;
        TomlTable *table_val;
    } data;
};

// Parser functions
TomlTable *toml_parse_file(const char *path);
TomlTable *toml_parse_string(const char *content);
void toml_free_table(TomlTable *table);

// Access functions
TomlValue *toml_get(TomlTable *table, const char *key);
TomlValue *toml_get_path(TomlTable *table, const char *path);  // e.g., "docker.expose"

// Type-safe getters (return default if not found or wrong type)
const char *toml_get_string(TomlTable *table, const char *path, const char *default_val);
long long toml_get_int(TomlTable *table, const char *path, long long default_val);
int toml_get_bool(TomlTable *table, const char *path, int default_val);
TomlArray *toml_get_array(TomlTable *table, const char *path);
TomlTable *toml_get_table(TomlTable *table, const char *path);

// Array helpers
int toml_array_len(TomlArray *arr);
TomlValue *toml_array_get(TomlArray *arr, int index);
const char *toml_array_get_string(TomlArray *arr, int index);
long long toml_array_get_int(TomlArray *arr, int index);

#endif // ZC_TOML_H
