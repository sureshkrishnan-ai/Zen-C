// Zen-C Cloud Build - TOML Parser Implementation
// Lightweight TOML parser for zen.toml configuration

#include "toml.h"

// Internal parser state
typedef struct {
    const char *src;
    int pos;
    int line;
    int col;
} TomlParser;

// Forward declarations
static TomlValue *parse_value(TomlParser *p);
static void skip_whitespace(TomlParser *p);
static void skip_line(TomlParser *p);
static char peek(TomlParser *p);
static char advance(TomlParser *p);

// Memory allocation (use real malloc for cloud module, not arena)
#undef malloc
#undef free
#undef realloc
#undef calloc

static char *toml_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

// Create new table
static TomlTable *new_table(void) {
    TomlTable *t = (TomlTable *)calloc(1, sizeof(TomlTable));
    return t;
}

// Create new array
static TomlArray *new_array(void) {
    TomlArray *a = (TomlArray *)calloc(1, sizeof(TomlArray));
    a->capacity = 8;
    a->items = (TomlValue **)calloc(a->capacity, sizeof(TomlValue *));
    return a;
}

// Create new value
static TomlValue *new_value(TomlType type) {
    TomlValue *v = (TomlValue *)calloc(1, sizeof(TomlValue));
    v->type = type;
    return v;
}

// Add entry to table
static void table_set(TomlTable *table, const char *key, TomlValue *value) {
    TomlEntry *entry = (TomlEntry *)calloc(1, sizeof(TomlEntry));
    entry->key = toml_strdup(key);
    entry->value = value;
    entry->next = table->entries;
    table->entries = entry;
    table->count++;
}

// Add item to array
static void array_push(TomlArray *arr, TomlValue *value) {
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        arr->items = (TomlValue **)realloc(arr->items, arr->capacity * sizeof(TomlValue *));
    }
    arr->items[arr->count++] = value;
}

// Parser helpers
static char peek(TomlParser *p) {
    return p->src[p->pos];
}

static char advance(TomlParser *p) {
    char c = p->src[p->pos++];
    if (c == '\n') {
        p->line++;
        p->col = 1;
    } else {
        p->col++;
    }
    return c;
}

static void skip_whitespace(TomlParser *p) {
    while (peek(p) == ' ' || peek(p) == '\t') {
        advance(p);
    }
}

static void skip_whitespace_and_newlines(TomlParser *p) {
    while (1) {
        char c = peek(p);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(p);
        } else if (c == '#') {
            // Skip comment
            while (peek(p) && peek(p) != '\n') {
                advance(p);
            }
        } else {
            break;
        }
    }
}

static void skip_line(TomlParser *p) {
    while (peek(p) && peek(p) != '\n') {
        advance(p);
    }
    if (peek(p) == '\n') {
        advance(p);
    }
}

// Parse string (basic and literal)
static char *parse_string(TomlParser *p) {
    char quote = advance(p); // consume opening quote

    int capacity = 64;
    char *buf = (char *)malloc(capacity);
    int len = 0;

    while (peek(p) && peek(p) != quote) {
        if (len >= capacity - 1) {
            capacity *= 2;
            buf = (char *)realloc(buf, capacity);
        }

        char c = advance(p);
        if (c == '\\' && quote == '"') {
            // Handle escape sequences
            char next = advance(p);
            switch (next) {
                case 'n': buf[len++] = '\n'; break;
                case 't': buf[len++] = '\t'; break;
                case 'r': buf[len++] = '\r'; break;
                case '\\': buf[len++] = '\\'; break;
                case '"': buf[len++] = '"'; break;
                default: buf[len++] = next; break;
            }
        } else {
            buf[len++] = c;
        }
    }

    if (peek(p) == quote) {
        advance(p); // consume closing quote
    }

    buf[len] = '\0';
    return buf;
}

// Parse identifier/key
static char *parse_key(TomlParser *p) {
    int capacity = 32;
    char *buf = (char *)malloc(capacity);
    int len = 0;

    while (1) {
        char c = peek(p);
        if (isalnum(c) || c == '_' || c == '-') {
            if (len >= capacity - 1) {
                capacity *= 2;
                buf = (char *)realloc(buf, capacity);
            }
            buf[len++] = advance(p);
        } else {
            break;
        }
    }

    buf[len] = '\0';
    return buf;
}

// Parse integer
static long long parse_integer(TomlParser *p) {
    long long value = 0;
    int sign = 1;

    if (peek(p) == '-') {
        sign = -1;
        advance(p);
    } else if (peek(p) == '+') {
        advance(p);
    }

    while (isdigit(peek(p))) {
        value = value * 10 + (advance(p) - '0');
    }

    return sign * value;
}

// Parse array
static TomlArray *parse_array(TomlParser *p) {
    advance(p); // consume '['
    TomlArray *arr = new_array();

    skip_whitespace_and_newlines(p);

    while (peek(p) && peek(p) != ']') {
        TomlValue *val = parse_value(p);
        if (val) {
            array_push(arr, val);
        }

        skip_whitespace_and_newlines(p);

        if (peek(p) == ',') {
            advance(p);
            skip_whitespace_and_newlines(p);
        }
    }

    if (peek(p) == ']') {
        advance(p);
    }

    return arr;
}

// Parse inline table { key = value, ... }
static TomlTable *parse_inline_table(TomlParser *p) {
    advance(p); // consume '{'
    TomlTable *table = new_table();

    skip_whitespace(p);

    while (peek(p) && peek(p) != '}') {
        char *key = parse_key(p);
        skip_whitespace(p);

        if (peek(p) == '=') {
            advance(p);
            skip_whitespace(p);
            TomlValue *val = parse_value(p);
            if (val) {
                table_set(table, key, val);
            }
        }

        free(key);
        skip_whitespace(p);

        if (peek(p) == ',') {
            advance(p);
            skip_whitespace(p);
        }
    }

    if (peek(p) == '}') {
        advance(p);
    }

    return table;
}

// Parse a value
static TomlValue *parse_value(TomlParser *p) {
    skip_whitespace(p);
    char c = peek(p);

    if (c == '"' || c == '\'') {
        // String
        TomlValue *v = new_value(TOML_STRING);
        v->data.string_val = parse_string(p);
        return v;
    }
    else if (c == '[') {
        // Array
        TomlValue *v = new_value(TOML_ARRAY);
        v->data.array_val = parse_array(p);
        return v;
    }
    else if (c == '{') {
        // Inline table
        TomlValue *v = new_value(TOML_TABLE);
        v->data.table_val = parse_inline_table(p);
        return v;
    }
    else if (c == 't' && strncmp(p->src + p->pos, "true", 4) == 0) {
        // Boolean true
        p->pos += 4;
        TomlValue *v = new_value(TOML_BOOL);
        v->data.bool_val = 1;
        return v;
    }
    else if (c == 'f' && strncmp(p->src + p->pos, "false", 5) == 0) {
        // Boolean false
        p->pos += 5;
        TomlValue *v = new_value(TOML_BOOL);
        v->data.bool_val = 0;
        return v;
    }
    else if (isdigit(c) || c == '-' || c == '+') {
        // Integer
        TomlValue *v = new_value(TOML_INT);
        v->data.int_val = parse_integer(p);
        return v;
    }

    return NULL;
}

// Get or create nested table
static TomlTable *get_or_create_table(TomlTable *root, const char *path) {
    char *path_copy = toml_strdup(path);
    char *token = strtok(path_copy, ".");
    TomlTable *current = root;

    while (token) {
        TomlValue *existing = toml_get(current, token);
        if (existing && existing->type == TOML_TABLE) {
            current = existing->data.table_val;
        } else {
            // Create new table
            TomlValue *new_tbl = new_value(TOML_TABLE);
            new_tbl->data.table_val = new_table();
            table_set(current, token, new_tbl);
            current = new_tbl->data.table_val;
        }
        token = strtok(NULL, ".");
    }

    free(path_copy);
    return current;
}

// Parse TOML document
TomlTable *toml_parse_string(const char *content) {
    if (!content) return NULL;

    TomlParser p = {0};
    p.src = content;
    p.pos = 0;
    p.line = 1;
    p.col = 1;

    TomlTable *root = new_table();
    TomlTable *current_table = root;

    while (peek(&p)) {
        skip_whitespace_and_newlines(&p);

        if (!peek(&p)) break;

        char c = peek(&p);

        if (c == '#') {
            // Comment
            skip_line(&p);
        }
        else if (c == '[') {
            // Table header
            advance(&p);

            // Check for array of tables [[...]]
            int is_array_table = 0;
            if (peek(&p) == '[') {
                advance(&p);
                is_array_table = 1;
            }

            // Parse table name
            int capacity = 64;
            char *name = (char *)malloc(capacity);
            int len = 0;

            while (peek(&p) && peek(&p) != ']') {
                if (len >= capacity - 1) {
                    capacity *= 2;
                    name = (char *)realloc(name, capacity);
                }
                name[len++] = advance(&p);
            }
            name[len] = '\0';

            // Skip closing bracket(s)
            if (peek(&p) == ']') advance(&p);
            if (is_array_table && peek(&p) == ']') advance(&p);

            // Get or create the table
            current_table = get_or_create_table(root, name);
            free(name);

            skip_line(&p);
        }
        else if (isalnum(c) || c == '_' || c == '"') {
            // Key = value
            char *key;
            if (c == '"') {
                key = parse_string(&p);
            } else {
                key = parse_key(&p);
            }

            skip_whitespace(&p);

            if (peek(&p) == '.') {
                // Dotted key like kubernetes.resources.cpu
                int capacity = 128;
                char *full_key = (char *)malloc(capacity);
                strcpy(full_key, key);
                int len = strlen(key);
                free(key);

                while (peek(&p) == '.') {
                    advance(&p);
                    char *next_part = parse_key(&p);
                    int next_len = strlen(next_part);

                    if (len + next_len + 2 >= capacity) {
                        capacity *= 2;
                        full_key = (char *)realloc(full_key, capacity);
                    }

                    full_key[len++] = '.';
                    strcpy(full_key + len, next_part);
                    len += next_len;
                    free(next_part);
                }

                skip_whitespace(&p);

                if (peek(&p) == '=') {
                    advance(&p);
                    skip_whitespace(&p);
                    TomlValue *val = parse_value(&p);

                    // Find the parent table and set the final key
                    char *last_dot = strrchr(full_key, '.');
                    if (last_dot) {
                        *last_dot = '\0';
                        TomlTable *parent = get_or_create_table(current_table, full_key);
                        table_set(parent, last_dot + 1, val);
                    } else {
                        table_set(current_table, full_key, val);
                    }
                }

                free(full_key);
            }
            else if (peek(&p) == '=') {
                advance(&p);
                skip_whitespace(&p);
                TomlValue *val = parse_value(&p);
                if (val) {
                    table_set(current_table, key, val);
                }
                free(key);
            } else {
                free(key);
            }

            skip_line(&p);
        }
        else {
            advance(&p);
        }
    }

    return root;
}

// Parse TOML file
TomlTable *toml_parse_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = (char *)malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    TomlTable *result = toml_parse_string(content);
    free(content);
    return result;
}

// Free functions
static void free_value(TomlValue *v);

static void free_array(TomlArray *arr) {
    if (!arr) return;
    for (int i = 0; i < arr->count; i++) {
        free_value(arr->items[i]);
    }
    free(arr->items);
    free(arr);
}

static void free_table(TomlTable *table) {
    if (!table) return;
    TomlEntry *entry = table->entries;
    while (entry) {
        TomlEntry *next = entry->next;
        free(entry->key);
        free_value(entry->value);
        free(entry);
        entry = next;
    }
    free(table);
}

static void free_value(TomlValue *v) {
    if (!v) return;
    switch (v->type) {
        case TOML_STRING:
            free(v->data.string_val);
            break;
        case TOML_ARRAY:
            free_array(v->data.array_val);
            break;
        case TOML_TABLE:
            free_table(v->data.table_val);
            break;
        default:
            break;
    }
    free(v);
}

void toml_free_table(TomlTable *table) {
    free_table(table);
}

// Access functions
TomlValue *toml_get(TomlTable *table, const char *key) {
    if (!table || !key) return NULL;

    TomlEntry *entry = table->entries;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

TomlValue *toml_get_path(TomlTable *table, const char *path) {
    if (!table || !path) return NULL;

    char *path_copy = toml_strdup(path);
    char *token = strtok(path_copy, ".");
    TomlValue *current = NULL;
    TomlTable *current_table = table;

    while (token && current_table) {
        current = toml_get(current_table, token);
        if (!current) {
            free(path_copy);
            return NULL;
        }

        token = strtok(NULL, ".");
        if (token) {
            if (current->type != TOML_TABLE) {
                free(path_copy);
                return NULL;
            }
            current_table = current->data.table_val;
        }
    }

    free(path_copy);
    return current;
}

// Type-safe getters
const char *toml_get_string(TomlTable *table, const char *path, const char *default_val) {
    TomlValue *v = toml_get_path(table, path);
    if (v && v->type == TOML_STRING) {
        return v->data.string_val;
    }
    return default_val;
}

long long toml_get_int(TomlTable *table, const char *path, long long default_val) {
    TomlValue *v = toml_get_path(table, path);
    if (v && v->type == TOML_INT) {
        return v->data.int_val;
    }
    return default_val;
}

int toml_get_bool(TomlTable *table, const char *path, int default_val) {
    TomlValue *v = toml_get_path(table, path);
    if (v && v->type == TOML_BOOL) {
        return v->data.bool_val;
    }
    return default_val;
}

TomlArray *toml_get_array(TomlTable *table, const char *path) {
    TomlValue *v = toml_get_path(table, path);
    if (v && v->type == TOML_ARRAY) {
        return v->data.array_val;
    }
    return NULL;
}

TomlTable *toml_get_table(TomlTable *table, const char *path) {
    TomlValue *v = toml_get_path(table, path);
    if (v && v->type == TOML_TABLE) {
        return v->data.table_val;
    }
    return NULL;
}

// Array helpers
int toml_array_len(TomlArray *arr) {
    return arr ? arr->count : 0;
}

TomlValue *toml_array_get(TomlArray *arr, int index) {
    if (!arr || index < 0 || index >= arr->count) {
        return NULL;
    }
    return arr->items[index];
}

const char *toml_array_get_string(TomlArray *arr, int index) {
    TomlValue *v = toml_array_get(arr, index);
    if (v && v->type == TOML_STRING) {
        return v->data.string_val;
    }
    return NULL;
}

long long toml_array_get_int(TomlArray *arr, int index) {
    TomlValue *v = toml_array_get(arr, index);
    if (v && v->type == TOML_INT) {
        return v->data.int_val;
    }
    return 0;
}
