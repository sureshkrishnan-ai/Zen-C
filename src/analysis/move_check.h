#ifndef MOVE_CHECK_H
#define MOVE_CHECK_H

#include "../parser/parser.h"
#include "typecheck.h"

// Forward declaration
struct TypeChecker;

/**
 * @brief Status of a moved variable in a specific flow path.
 */
typedef enum
{
    MOVE_STATE_VALID,
    MOVE_STATE_MOVED,
    MOVE_STATE_MAYBE_MOVED // Used when merging diverging paths (e.g., if/else)
} MoveStatus;

/**
 * @brief Linked list of tracked moved symbols in the current state.
 */
typedef struct MoveEntry
{
    char *symbol_name;
    MoveStatus status;
    Token moved_at; // For error reporting
    struct MoveEntry *next;
} MoveEntry;

/**
 * @brief Represents the state of moves at a specific point in control flow.
 */
typedef struct MoveState
{
    struct MoveEntry *entries; // List of tracked symbols
    struct MoveState *parent;  // Parent state (for scoping/forking)
} MoveState;

/**
 * @brief Creates a new move state, optionally inheriting from a parent.
 */
MoveState *move_state_create(MoveState *parent);

/**
 * @brief Deep clones a move state (for branching).
 */
MoveState *move_state_clone(MoveState *src);

/**
 * @brief Merges two branches into a target state.
 *
 * Logic:
 * - Valid + Valid -> Valid
 * - Moved + Moved -> Moved
 * - Moved + Valid -> Maybe Moved (Error on use)
 */
void move_state_merge(MoveState *target, MoveState *a, MoveState *b);

/**
 * @brief Frees a move state.
 */
void move_state_free(MoveState *state);

/**
 * @brief Check if a symbol is moved in the given state (or parents).
 */
MoveStatus get_move_status(MoveState *state, const char *name);

/**
 * @brief Determines if a type is safe to copy (implements Copy or is a primitive).
 *
 * @param ctx Parser context for type lookups.
 * @param t The type to check.
 * @return 1 if the type is Copy, 0 if it is Move.
 */
int is_type_copy(ParserContext *ctx, Type *t);

/**
 * @brief Checks if a symbol uses is valid (not moved).
 *
 * Reports an error if the symbol has been moved.
 *
 * @param tc Type checker context for error reporting.
 * @param node The AST node where the use occurs (for location).
 * @param sym The symbol being used.
 */
void check_use_validity(TypeChecker *tc, ASTNode *node, ZenSymbol *sym);

/**
 * @brief Marks a symbol as moved if its type is not Copy.
 *
 * @param ctx Parser context (for checking Copy trait).
 * @param sym The symbol to mark.
 * @param context_node The AST node causing the move (assignment, call, etc.).
 */
void mark_symbol_moved(ParserContext *ctx, ZenSymbol *sym, ASTNode *context_node);

/**
 * @brief Marks a symbol as valid (initialized or re-assigned).
 *
 * @param sym The symbol to mark.
 */
void mark_symbol_valid(ZenSymbol *sym);

#endif // MOVE_CHECK_H
