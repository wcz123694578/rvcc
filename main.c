#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    TK_PUNCT,   // 操作符
    TK_NUM,
    TK_EOF
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind Kind;
    Token *Next;
    int Val;
    char *Loc;      // 在字符串中的位置
    int Len;
};

static char *CurrentInput;

static void error(char *fmt, ...) {
    va_list VA;
    va_start(VA, fmt);
    vfprintf(stderr, fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
    exit(1);
}

static void verrorAt(char *Loc, char *Fmt, va_list VA) {
    fprintf(stderr, "%s\n", CurrentInput);

    int Pos = Loc - CurrentInput;
    fprintf(stderr, "%*s", Pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, Fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
    exit(1);
}

static void errorAt(char *Loc, char *Fmt, ...) {
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Loc, Fmt, VA);
}

static void errorTok(Token *Tok, char *Fmt, ...) {
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Tok->Loc, Fmt, VA);
}

static bool equal(Token *Tok, char *Str) {
    return memcmp(Tok->Loc, Str, Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

static Token *newToken(TokenKind Kind, char *Start, char *End) {
    Token *Tok = calloc(1, sizeof(Token));
    Tok->Kind = Kind;
    Tok->Loc = Start;
    Tok->Len = End - Start;
    return Tok;
}

// 跳过指定的Str
static Token *skip(Token *Tok, char *Str) {
    if (!equal(Tok, Str))
        errorTok(Tok, "expect '%s'", Str);
    return Tok->Next;
}

static int getNumber(Token *Tok) {
  if (Tok->Kind != TK_NUM)
    error("expect a number");
  return Tok->Val;
}

static Token *tokenize() {
    char *P = CurrentInput;

    Token Head = {};
    Token *Cur = &Head;

    while (*P) {
        if (isspace(*P)) {      // 终结符判断
            ++P;
            continue;
        }

        if (isdigit(*P)) {
            Cur->Next = newToken(TK_NUM, P, P);
            Cur = Cur->Next;
            const char *OldPtr = P;
            Cur->Val = strtoul(P, &P, 10);
            Cur->Len = P - OldPtr;
            continue;
        }

        if (*P == '+' || *P == '-') {
            Cur->Next = newToken(TK_PUNCT, P, P + 1);
            Cur = Cur->Next;
            ++P;
            continue;
        }

        errorAt(P, "invalid token");
    }

    Cur->Next = newToken(TK_EOF, P, P);
    return Head.Next;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("%s: invalid number of arguments.", argv[0]);
    }

    CurrentInput = argv[1];
    Token *Tok = tokenize();

    printf("    .global main\n");
    printf("main:\n");
    printf("    li a0, %d\n", getNumber(Tok));
    Tok = Tok->Next;

    while (Tok->Kind != TK_EOF) {
        if (equal(Tok, "+")) {
            Tok = Tok->Next;
            printf("    addi a0, a0, %d\n", getNumber(Tok));
            Tok = Tok->Next;
            continue;
        }

        Tok = skip(Tok, "-");
        printf("    addi a0, a0, -%d\n", getNumber(Tok));
        Tok = Tok->Next;
    }

    printf("    ret\n");

    return 0;
}