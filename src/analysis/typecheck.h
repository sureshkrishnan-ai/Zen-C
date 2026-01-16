#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "ast.h"
#include "parser.h"

// Type Checker Context
// Holds the state during the semantic analysis pass.
// Unlike the parser, this focuses on semantic validity (types, definitions).
typedef struct TypeChecker
{
    ParserContext *pctx;   // Reference to global parser context (for lookups)
    Scope *current_scope;  // Current lexical scope
    ASTNode *current_func; // Current function being checked (for return type checks)
    int error_count;       // Number of errors found
    int warning_count;     // Number of recommendations/warnings
} TypeChecker;

// Main Entry Point
// Returns 0 on success (no errors), non-zero if errors occurred.
int check_program(ParserContext *ctx, ASTNode *root);

#endif // TYPECHECK_H
