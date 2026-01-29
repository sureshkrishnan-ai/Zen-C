
#include "borrow_check.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ** Internal Helpers **

static void bc_error(BorrowChecker *bc, Token t, const char *fmt, ...)
{
    fprintf(stderr, "Borrow Error at %s:%d:%d: ", g_current_filename, t.line, t.col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    bc->error_count++;
}

static void bc_add_borrow(BorrowChecker *bc, const char *var_name,
                           const char *borrower_name, int is_mutable, Token t)
{
    BorrowEntry *entry = malloc(sizeof(BorrowEntry));
    if (!entry)
    {
        return;
    }
    entry->var_name = strdup(var_name);
    entry->borrower_name = borrower_name ? strdup(borrower_name) : NULL;
    entry->is_mutable = is_mutable;
    entry->scope_depth = bc->scope_depth;
    entry->borrow_token = t;
    entry->next = bc->active_borrows;
    bc->active_borrows = entry;
}

static void bc_release_scope(BorrowChecker *bc, int depth)
{
    BorrowEntry **pp = &bc->active_borrows;
    while (*pp)
    {
        if ((*pp)->scope_depth >= depth)
        {
            BorrowEntry *old = *pp;
            *pp = old->next;
            free(old->var_name);
            free(old->borrower_name);
            free(old);
        }
        else
        {
            pp = &(*pp)->next;
        }
    }
}

/**
 * @brief Check if a new borrow of var_name is allowed.
 *
 * Rules:
 * - If requesting &mut: no existing borrows (mutable or immutable) allowed
 * - If requesting &: no existing &mut borrows allowed
 */
static int bc_check_borrow_allowed(BorrowChecker *bc, const char *var_name,
                                    int is_mutable, Token t)
{
    int has_immutable = 0;
    int has_mutable = 0;
    Token existing_tok = {0};

    BorrowEntry *e = bc->active_borrows;
    while (e)
    {
        if (strcmp(e->var_name, var_name) == 0)
        {
            if (e->is_mutable)
            {
                has_mutable = 1;
                existing_tok = e->borrow_token;
            }
            else
            {
                has_immutable = 1;
                existing_tok = e->borrow_token;
            }
        }
        e = e->next;
    }

    if (is_mutable)
    {
        if (has_mutable)
        {
            bc_error(bc, t,
                     "Cannot borrow '%s' as mutable more than once at a time "
                     "(previous mutable borrow at line %d)",
                     var_name, existing_tok.line);
            return 0;
        }
        if (has_immutable)
        {
            bc_error(bc, t,
                     "Cannot borrow '%s' as mutable while it is borrowed as immutable "
                     "(immutable borrow at line %d)",
                     var_name, existing_tok.line);
            return 0;
        }
    }
    else
    {
        if (has_mutable)
        {
            bc_error(bc, t,
                     "Cannot borrow '%s' as immutable while it is borrowed as mutable "
                     "(mutable borrow at line %d)",
                     var_name, existing_tok.line);
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Check if a variable is currently borrowed (for mutation guard).
 */
static int bc_is_borrowed(BorrowChecker *bc, const char *var_name)
{
    BorrowEntry *e = bc->active_borrows;
    while (e)
    {
        if (strcmp(e->var_name, var_name) == 0)
        {
            return 1;
        }
        e = e->next;
    }
    return 0;
}

/**
 * @brief Check if a borrower holds an immutable reference (for write guard).
 */
static int bc_is_immutable_ref(BorrowChecker *bc, const char *borrower_name)
{
    BorrowEntry *e = bc->active_borrows;
    while (e)
    {
        if (e->borrower_name && strcmp(e->borrower_name, borrower_name) == 0)
        {
            return !e->is_mutable;
        }
        e = e->next;
    }
    return 0;
}

// ** Helpers to extract borrow info from expressions **

/**
 * @brief Extract the variable name from a unary & expression.
 * Returns NULL if the operand is not a simple variable.
 */
static const char *get_borrowed_var(ASTNode *expr)
{
    if (!expr)
    {
        return NULL;
    }
    if (expr->type == NODE_EXPR_UNARY &&
        expr->unary.op && expr->unary.op[0] == '&' &&
        expr->unary.operand &&
        expr->unary.operand->type == NODE_EXPR_VAR)
    {
        return expr->unary.operand->var_ref.name;
    }
    return NULL;
}

/**
 * @brief Check if a unary & expression is a mutable borrow.
 * Determined by the type annotation on the variable declaration.
 */
static int is_mutable_borrow_type(Type *type_info)
{
    if (!type_info)
    {
        return 0;
    }
    if (type_info->kind == TYPE_REF || type_info->kind == TYPE_REF_SLICE)
    {
        return type_info->is_mutable;
    }
    return 0;
}

// ** AST Walk **

static void bc_check_node(BorrowChecker *bc, ASTNode *node);

static void bc_check_block(BorrowChecker *bc, ASTNode *node)
{
    bc->scope_depth++;
    ASTNode *stmt = node->block.statements;
    while (stmt)
    {
        bc_check_node(bc, stmt);
        stmt = stmt->next;
    }
    bc_release_scope(bc, bc->scope_depth);
    bc->scope_depth--;
}

static void bc_check_var_decl(BorrowChecker *bc, ASTNode *node)
{
    if (!node->var_decl.init_expr)
    {
        return;
    }

    // Check if this is a borrow: let r: &T = &x;
    const char *borrowed_var = get_borrowed_var(node->var_decl.init_expr);
    if (borrowed_var)
    {
        int is_mut = is_mutable_borrow_type(node->type_info);

        // Check borrow rules
        if (bc_check_borrow_allowed(bc, borrowed_var, is_mut, node->token))
        {
            bc_add_borrow(bc, borrowed_var, node->var_decl.name, is_mut, node->token);
        }
    }

    // Recurse into init expression
    bc_check_node(bc, node->var_decl.init_expr);
}

static void bc_check_binary(BorrowChecker *bc, ASTNode *node)
{
    bc_check_node(bc, node->binary.left);
    bc_check_node(bc, node->binary.right);

    // Check assignment through immutable reference: *r = value
    if (strcmp(node->binary.op, "=") == 0)
    {
        ASTNode *lhs = node->binary.left;

        // Deref assignment: *ref = value
        if (lhs->type == NODE_EXPR_UNARY && lhs->unary.op &&
            strcmp(lhs->unary.op, "*") == 0 &&
            lhs->unary.operand && lhs->unary.operand->type == NODE_EXPR_VAR)
        {
            const char *ref_name = lhs->unary.operand->var_ref.name;
            if (bc_is_immutable_ref(bc, ref_name))
            {
                bc_error(bc, node->token,
                         "Cannot assign through immutable reference '%s' "
                         "(use &mut for mutable borrow)",
                         ref_name);
            }
        }

        // Direct assignment to borrowed variable: x = value (while x is borrowed)
        if (lhs->type == NODE_EXPR_VAR)
        {
            const char *var_name = lhs->var_ref.name;
            if (bc_is_borrowed(bc, var_name))
            {
                bc_error(bc, node->token,
                         "Cannot assign to '%s' while it is borrowed",
                         var_name);
            }
        }
    }
}

static void bc_check_function(BorrowChecker *bc, ASTNode *node)
{
    // Each function gets a clean borrow state
    BorrowEntry *saved = bc->active_borrows;
    int saved_depth = bc->scope_depth;
    bc->active_borrows = NULL;
    bc->scope_depth = 0;

    bc_check_node(bc, node->func.body);

    // Release any remaining borrows from function
    bc_release_scope(bc, 0);
    bc->active_borrows = saved;
    bc->scope_depth = saved_depth;
}

static void bc_check_node(BorrowChecker *bc, ASTNode *node)
{
    if (!node)
    {
        return;
    }

    switch (node->type)
    {
    case NODE_ROOT:
        bc_check_node(bc, node->root.children);
        break;

    case NODE_BLOCK:
        bc_check_block(bc, node);
        break;

    case NODE_VAR_DECL:
        bc_check_var_decl(bc, node);
        break;

    case NODE_FUNCTION:
        bc_check_function(bc, node);
        break;

    case NODE_EXPR_BINARY:
        bc_check_binary(bc, node);
        break;

    case NODE_IF:
        bc_check_node(bc, node->if_stmt.condition);
        bc_check_node(bc, node->if_stmt.then_body);
        bc_check_node(bc, node->if_stmt.else_body);
        break;

    case NODE_WHILE:
        bc_check_node(bc, node->while_stmt.condition);
        bc_check_node(bc, node->while_stmt.body);
        break;

    case NODE_FOR:
        bc->scope_depth++;
        bc_check_node(bc, node->for_stmt.init);
        bc_check_node(bc, node->for_stmt.condition);
        bc_check_node(bc, node->for_stmt.step);
        bc_check_node(bc, node->for_stmt.body);
        bc_release_scope(bc, bc->scope_depth);
        bc->scope_depth--;
        break;

    case NODE_RETURN:
        if (node->ret.value)
        {
            bc_check_node(bc, node->ret.value);
        }
        break;

    case NODE_EXPR_CALL:
        bc_check_node(bc, node->call.callee);
        {
            ASTNode *arg = node->call.args;
            while (arg)
            {
                bc_check_node(bc, arg);
                arg = arg->next;
            }
        }
        break;

    case NODE_EXPR_UNARY:
        bc_check_node(bc, node->unary.operand);
        break;

    case NODE_EXPR_VAR:
        // Variable use - no special borrow check needed here
        break;

    case NODE_MATCH:
        bc_check_node(bc, node->match_stmt.expr);
        {
            ASTNode *c = node->match_stmt.cases;
            while (c)
            {
                bc_check_node(bc, c);
                c = c->next;
            }
        }
        break;

    case NODE_MATCH_CASE:
        bc_check_node(bc, node->match_case.body);
        break;

    case NODE_LOOP:
        bc_check_node(bc, node->loop_stmt.body);
        break;

    case NODE_TEST:
        bc_check_function(bc, node); // Tests are like functions for borrow scope
        break;

    default:
        break;
    }

    // Continue to next sibling
    if (node->next)
    {
        bc_check_node(bc, node->next);
    }
}

// ** Entry Point **

int check_borrows(ParserContext *ctx, ASTNode *root)
{
    BorrowChecker bc = {0};
    bc.pctx = ctx;

    bc_check_node(&bc, root);

    if (bc.error_count > 0)
    {
        fprintf(stderr, "[BorrowCheck] Found %d borrow violation(s).\n", bc.error_count);
        return 1;
    }
    return 0;
}
