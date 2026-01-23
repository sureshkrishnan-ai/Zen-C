
#ifndef ZEN_CONSTANTS_H
#define ZEN_CONSTANTS_H

// Buffer sizes
#define MAX_TYPE_NAME_LEN 256
#define MAX_FUNC_NAME_LEN 512
#define MAX_ERROR_MSG_LEN 1024
#define MAX_MANGLED_NAME_LEN 512
#define MAX_PATH_LEN 4096

// Type checking helpers
#define IS_INT_TYPE(t) ((t) && strcmp((t), "int") == 0)
#define IS_BOOL_TYPE(t) ((t) && strcmp((t), "bool") == 0)
#define IS_CHAR_TYPE(t) ((t) && strcmp((t), "char") == 0)
#define IS_VOID_TYPE(t) ((t) && strcmp((t), "void") == 0)
#define IS_FLOAT_TYPE(t) ((t) && strcmp((t), "float") == 0)
#define IS_DOUBLE_TYPE(t) ((t) && strcmp((t), "double") == 0)
#define IS_USIZE_TYPE(t) ((t) && (strcmp((t), "usize") == 0 || strcmp((t), "size_t") == 0))
#define IS_STRING_TYPE(t)                                                                          \
    ((t) &&                                                                                        \
     (strcmp((t), "string") == 0 || strcmp((t), "char*") == 0 || strcmp((t), "const char*") == 0))

// Composite type checks
#define IS_BASIC_TYPE(t)                                                                           \
    ((t) && (IS_INT_TYPE(t) || IS_BOOL_TYPE(t) || IS_CHAR_TYPE(t) || IS_VOID_TYPE(t) ||            \
             IS_FLOAT_TYPE(t) || IS_DOUBLE_TYPE(t) || IS_USIZE_TYPE(t) ||                          \
             strcmp((t), "ssize_t") == 0 || strcmp((t), "__auto_type") == 0))

#define IS_NUMERIC_TYPE(t)                                                                         \
    ((t) && (IS_INT_TYPE(t) || IS_FLOAT_TYPE(t) || IS_DOUBLE_TYPE(t) || IS_USIZE_TYPE(t)))

// Pointer type check
#define IS_PTR_TYPE(t) ((t) && strchr((t), '*') != NULL)

// Struct prefix check
#define IS_STRUCT_PREFIX(t) ((t) && strncmp((t), "struct ", 7) == 0)
#define STRIP_STRUCT_PREFIX(t) (IS_STRUCT_PREFIX(t) ? ((t) + 7) : (t))

// Generic type checks
#define IS_OPTION_TYPE(t) ((t) && strncmp((t), "Option_", 7) == 0)
#define IS_RESULT_TYPE(t) ((t) && strncmp((t), "Result_", 7) == 0)
#define IS_VEC_TYPE(t) ((t) && strncmp((t), "Vec_", 4) == 0)
#define IS_SLICE_TYPE(t) ((t) && strncmp((t), "Slice_", 6) == 0)

#endif // ZEN_CONSTANTS_H
