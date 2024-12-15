#include "rvcc.h"

static char *CurrentInput;

void error(char *fmt, ...) {
    va_list VA;
    va_start(VA, fmt);
    vfprintf(stderr, fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
    exit(1);
}

void verrorAt(char *Loc, char *Fmt, va_list VA) {
    fprintf(stderr, "%s\n", CurrentInput);

    int Pos = Loc - CurrentInput;
    fprintf(stderr, "%*s", Pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, Fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
    exit(1);
}

void errorAt(char *Loc, char *Fmt, ...) {
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Loc, Fmt, VA);
}

void errorTok(Token *Tok, char *Fmt, ...) {
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Tok->Loc, Fmt, VA);
}

bool equal(Token *Tok, char *Str) {
    return memcmp(Tok->Loc, Str, Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

Token *newToken(TokenKind Kind, char *Start, char *End) {
    Token *Tok = calloc(1, sizeof(Token));
    Tok->Kind = Kind;
    Tok->Loc = Start;
    Tok->Len = End - Start;
    return Tok;
}

// 跳过指定的Str
Token *skip(Token *Tok, char *Str) {
    if (!equal(Tok, Str))
        errorTok(Tok, "expect '%s'", Str);
    return Tok->Next;
}

// 消耗掉指定Token
bool consume(Token **Rest, Token *Tok, char *Str) {
    // 存在
    if (equal(Tok, Str)) {
        *Rest = Tok->Next;
        return true;
    }
    // 不存在
    *Rest = Tok;
    return false;
}

bool startsWith(char *Str, char *SubStr) {
    return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

// 判断标记符的首字母规则
// [a-zA-Z_]
static bool isIdent1(char C) {
    // a-z与A-Z在ASCII中不相连，所以需要分别判断
    return ('a' <= C && C <= 'z') || ('A' <= C && C <= 'Z') || C == '_';
}
// 判断标记符的非首字母的规则
// [a-zA-Z0-9_]
static bool isIdent2(char C) { return isIdent1(C) || ('0' <= C && C <= '9'); }

int readPunct(char *Ptr) {
    if (    startsWith(Ptr, "==") 
        ||  startsWith(Ptr, "!=")
        ||  startsWith(Ptr, "<=")
        ||  startsWith(Ptr, ">="))
    {
        return 2;    
    }

    return ispunct(*Ptr) ? 1 : 0;
}

// 判断是否为关键字
static bool isKeyword(Token *Tok) {
    // 关键字列表
    static char *Kw[] = { 
        "return", 
        "if", 
        "else", 
        "for", 
        "while", 
        "int", 
        "sizeof",
        "char"
    };
    // 遍历关键字列表匹配
    for (int I = 0; I < sizeof(Kw) / sizeof(*Kw); ++I) {
        if (equal(Tok, Kw[I]))
        return true;
    }
    return false;
}

// 将名为“return”的终结符转为KEYWORD
static void convertKeywords(Token *Tok) {
    for (Token *T = Tok; T->Kind != TK_EOF; T = T->Next) {
        if (isKeyword(T))
        T->Kind = TK_KEYWORD;
    }
}

int getNumber(Token *Tok) {
    if (Tok->Kind != TK_NUM)
        error("expect a number");
    return Tok->Val;
}

Token *tokenize(char *P) {
    CurrentInput = P;

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

        if (isIdent1(*P)) {
            char *Start = P;
            do {
                ++P;
            } while (isIdent2(*P));
            Cur->Next = newToken(TK_IDENT, Start, P);
            Cur= Cur->Next;
            continue;
        }

        int PunctLen = readPunct(P);
        if (PunctLen) {
            Cur->Next = newToken(TK_PUNCT, P, P + PunctLen);
            Cur = Cur->Next;
            P += PunctLen;
            continue;
        }

        errorAt(P, "invalid token");
    }

    Cur->Next = newToken(TK_EOF, P, P);
    convertKeywords(Head.Next);
    return Head.Next;
}