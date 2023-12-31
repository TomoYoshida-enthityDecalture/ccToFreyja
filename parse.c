#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "ccToFreyja.h"
#include "parse.h"

Node *code[100];

LVar *locals;

void init_lvar() {
    locals = calloc(1, sizeof(LVar));
    locals->next = NULL;
    locals->name = NULL;
    locals->len = 0; 
    locals->offset = 0;
}

LVar *find_lvar(char *str, int len) {
    LVar *var = locals;
    while (var != NULL) {
        if (var->len == len && strncmp(str, var->name, var->len) == 0) return var;
        var = var->next;
    }
    return NULL;
}

//generate new node
Node *new_node(Nodetype type, Node *lhs, Node *rhs) {
    Node *new = calloc(1, sizeof(Node));
    new->type = type;
    new->lhs = lhs;
    new->rhs = rhs;
    
    return new;
}

//generate leaf num
Node *new_node_num(int val) {
    Node *new = calloc(1, sizeof(Node));
    new->type = ND_NUM;
    new->val = val;

    return new;
}

Node *new_node_ident(char *val, int len) {
    Node *new = calloc(1, sizeof(Node));
    new->type = ND_LVAR;
    
    LVar *lvar = find_lvar(val, len);
    if (lvar) {
        new->offset = lvar->offset;
    } else {
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = val;
        lvar->len = len;
        lvar->offset = locals->offset + 8;
        new->offset = lvar->offset;
        locals = lvar;
    }
    
    return new;
}

Node *new_node_func(char *val, int len) {
    Node *new = calloc(1, sizeof(Node));
    new->type = ND_FUNC;
    new->str = val;
    new->val = len;

    return new;
}

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relation();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt() {
    Node *node = NULL;

    if (consume("{")) {
        while (!consume("}")) {
            node = new_node(ND_BLOCK, node, stmt());
        }
    } else if (consume_keyword("if")) {
        expected("(");
        Node *ifn = expr();
        expected(")");
        Node *stm = stmt();
        if (consume_keyword("else")) {
            node = new_node(ND_IF, ifn, new_node(ND_ELSE, stm, stmt()));
        } else node = new_node(ND_IF, ifn, new_node(ND_ELSE, stm, NULL));
    } else if (consume_keyword("while")) {
        expected("(");
        Node *ifn = expr();
        expected(")");
        node = new_node(ND_WHILE, ifn, stmt());
    } else if (consume_keyword("for")) {
        expected("(");
        Node *for1 = NULL, *for2 = NULL, *for3 = NULL;
        if (current->next->str != ";") 
            for1 = expr();
        expected(";");
        if (current->next->str != ";") 
            for2 = expr();
        expected(";");
        if (current->next->str != ";") 
            for3 = expr(); 
        expected(")");
        node = new_node(ND_FOR, new_node(ND_FOR1, for1, for2), new_node(ND_FOR2, for3, stmt()));
    } else if (consume_keyword("return")) {
        node = new_node(ND_RETURN, expr(), NULL);
        expected(";");
    } else {
        node = expr();
        expected(";");
    }

    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();

    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

Node *equality() {
    Node *node = relation();

    while (1) {
        if (consume("==")) node = new_node(ND_EQL, node, relation());
        else if (consume("!=")) node = new_node(ND_NEQ, node, relation());
        else return node;
    }
}

Node *relation() {
    Node *node = add();

    while (1) {
        if (consume("<=")) node = new_node(ND_LEQ, node, add());
        else if (consume(">=")) node = new_node(ND_GEQ, node, add());
        else if (consume("<")) node = new_node(ND_LE, node, add());
        else if (consume(">")) node = new_node(ND_GE, node, add());
        else return node;
    }
}

Node *add() {
    Node *node = mul();

    while (1) {
        if (consume("+")) node = new_node(ND_ADD, node, mul());
        else if (consume("-")) node = new_node(ND_SUB, node, mul());
        else return node;
    }
}

Node *mul() {
    Node *node = unary();
    
    while (1) {
        if (consume("*")) node = new_node(ND_MUL, node, unary());
        else if (consume("/")) node = new_node(ND_DIV, node, unary());
        else return node;
    }
}

Node *unary() {
    if (consume("+")) {
        return primary();
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else return primary();
}

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expected(")");
        return node;
    } else if (expected_ident()) {
        char *valstr = consume_ident();
        int len = consume_ident_len();
        current = current->next;
        if (consume("(")) {
            Node *node = new_node_func(valstr, len);
            expected(")");
            return node;      
        } else return new_node_ident(valstr, len);
    } else {
        return new_node_num(expected_num());
    }
}
