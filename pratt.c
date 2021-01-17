// Toy Pratt expression parser.
//
// This is a top-down recursive parser using Vaughan Pratt operator precedence technique.
// It turns an infix expression into a reverse-polish one.
//
// It supports a prefix operator and a few left or right associative infix
// operators in different precedence classes.
//
// As the goal is to teach myself this technique the input language is minimal.
// There is notably no lexical analyzer.  All tokens are one ASCII character long
// and spaces between tokens are not allowed.
//
// Grammar: 
// program -> expr '\0'
// expr -> primary | expr binary_operator expr
// primary -> variable | unary_operator variable | '(' expr ')'
// variable -> 'A'..'Z' | 'a'..'z'
// binary_operator -> '+' | '-' | '*' | '/' | '='
// unary_operator -> '~'

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////
// UTILITIES

void panic(const char *msg, ...) {
    char fmt[1024];
    snprintf(fmt, sizeof(fmt), "fatal error: %s", msg);

    va_list vl;
    va_start(vl, msg);
    vprintf(fmt, vl);
    va_end(vl);

    exit(1);
}

//////////////////////////////////////////////////////////////////////////////
// PARSER

// Maximum number of characters in output buffer.
enum { MAX_OUTPUT = 1024 };

// Parser state.
typedef struct {
    const char *input;          // next input character
    char *output;               // next output character
    const char *output_end;     // one past last possible output character
} Parser;

static void parse_expr(Parser* p, int precedence);

// Append given character at end of output.
static inline void emit(Parser* p, char ch) {
    if (p->output == p->output_end) {
        panic("output overflow");
    }
    *p->output++ = ch;
}

// Extract next character from input.
static inline char next_char(Parser* p) {
    char ch = *p->input;
    if (ch != '\0') {
        p->input++;
    }
    return ch;
}

// Consume next character in input and panic if this is not the expected one.
static inline void expect(Parser *p, char expected) {
    char got = next_char(p);
    if (got != expected) {
        panic("expected '%c', got '%c'", expected, got);
    }
}

// Look ahead next character from input.
static inline char peek_next_char(const Parser* p) {
    return *p->input;
}

static void parse_primary(Parser* p) {
    char ch = next_char(p);
    switch (ch) {
    case '~':
        parse_primary(p);
        emit(p, '~');
        break;
    case '(':
        parse_expr(p, 0);
        expect(p, ')');
        break;
    default:
        if (isalpha((unsigned char)ch)) {
            emit(p, ch);
        } else {
            panic("unexpected character: %c", ch);
        }
        break;
    }
}

// Operator precedence and associativity.
// Precedences must have gaps between them because the recursive parsing call
// for a right associative operator passes its precedence minus one to achieve
// right-associativity.
typedef struct {
    int precedence;
    bool right_associative;
} Operator;

static const Operator assignment_op = { 1, true };
static const Operator additive_op = { 10, false };
static const Operator multiplicative_op = { 20, false };

// Return characteristics of given operator or NULL if the given character is
// not an operator.
static const Operator* operator_info(char ch) {
    const Operator* p = NULL;
    switch (ch) {
    case '=':
        p = &assignment_op;
        break;
    case '+':
    case '-':
        p = &additive_op;
        break;
    case '*':
    case '/':
        p = &multiplicative_op;
        break;
    }
    return p;
}

static void parse_expr(Parser* p, int precedence) {
    parse_primary(p);

    for (;;) {
        char ch = peek_next_char(p);
        const Operator* opi = operator_info(ch);
        
        if (opi == NULL || precedence >= opi->precedence) {
            return;
        }

        next_char(p);
        parse_expr(p, opi->precedence - (opi->right_associative ? 1 : 0));

        emit(p, ch);
    }
}

// Parser entry point.
void parse(const char* input, char* output) {
    Parser parser = { input, output, output + MAX_OUTPUT };
    parse_expr(&parser, 0);
    expect(&parser, '\0');
    emit(&parser, '\0');
}

//////////////////////////////////////////////////////////////////////////////
// TESTS

typedef struct {
    const char *input;
    const char *expected;
} TestCase;

int main(void) {
    const TestCase tests[] = {
        { "a", "a" },
        { "~a", "a~" },
        { "~~a", "a~~" },
        { "a+b", "ab+" },
        { "a*b", "ab*" },
        { "a*~b", "ab~*" },
        { "a+b+c", "ab+c+" },
        { "a+b-c", "ab+c-" },
        { "a-b+c", "ab-c+" },
        { "a*b*c", "ab*c*" },
        { "a=b=c", "abc==" },
        { "a+b*c", "abc*+" },
        { "(a+b)*c", "ab+c*" },
        { "a*b+c", "ab*c+" },
        { "a=b+c", "abc+=" },
        { NULL, NULL }
    };

    int failures = 0;
    for (const TestCase* p = tests; p->input != NULL; p++) {
        char output[MAX_OUTPUT];
        printf("TEST: parsing %s\n", p->input);
        parse(p->input, output);
        if (strcmp(p->expected, output) != 0) {
            printf("FAILURE: parse(%s) = %s, expected: %s\n", p->input, output, p->expected);
            failures++;
        }
    }
    printf("%s\n", failures ? "FAILURE!!" : "SUCCESS!!");

    return 0;
}

// vim: ts=4:sts=4:sw=4
