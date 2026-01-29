
#ifndef BORROW_CHECK_H
#define BORROW_CHECK_H

#include "ast.h"
#include "parser.h"

/**
 * @brief Entry for tracking an active borrow.
 */
typedef struct BorrowEntry
{
    char *var_name;            ///< Variable being borrowed.
    char *borrower_name;       ///< Variable holding the reference.
    int is_mutable;            ///< 1 for &mut, 0 for &.
    int scope_depth;           ///< Scope depth where borrow was created.
    Token borrow_token;        ///< Source location for error reporting.
    struct BorrowEntry *next;  ///< Next entry in linked list.
} BorrowEntry;

/**
 * @brief Borrow Checker Context.
 *
 * Tracks active borrows and enforces Rust-like borrow rules:
 * - Multiple &T borrows allowed simultaneously
 * - Only one &mut T borrow at a time
 * - &T and &mut T cannot coexist on the same variable
 */
typedef struct BorrowChecker
{
    ParserContext *pctx;         ///< Reference to parser context.
    BorrowEntry *active_borrows; ///< Linked list of active borrows.
    int scope_depth;             ///< Current scope nesting depth.
    int error_count;             ///< Number of borrow violations found.
} BorrowChecker;

/**
 * @brief Run borrow checking on the AST.
 *
 * @param ctx Parser context.
 * @param root Root AST node.
 * @return 0 on success, non-zero if violations found.
 */
int check_borrows(ParserContext *ctx, ASTNode *root);

#endif // BORROW_CHECK_H
