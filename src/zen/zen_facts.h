
#ifndef ZEN_FACTS_H
#define ZEN_FACTS_H

#include "../zprep.h"

typedef enum
{
    TRIGGER_GOTO,
    TRIGGER_POINTER_ARITH,
    TRIGGER_BITWISE,
    TRIGGER_RECURSION,
    TRIGGER_TERNARY,
    TRIGGER_ASM,
    TRIGGER_WHILE_TRUE,
    TRIGGER_MACRO,
    TRIGGER_VOID_PTR,
    TRIGGER_MAIN,
    TRIGGER_FORMAT_STRING,
    TRIGGER_STRUCT_PADDING,
    TRIGGER_GLOBAL
} ZenTrigger;

void zen_init(void);

int zen_trigger_at(ZenTrigger t, Token location);

void zen_trigger_global(void);

const char *zen_get_fact(ZenTrigger t);

#endif
