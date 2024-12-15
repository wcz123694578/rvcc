#include <assert.h>
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

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整形
    ND_NEG,
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
} NodeKind;

typedef struct Token Token;
struct Token {
    TokenKind Kind;
    Token *Next;
    int Val;
    char *Loc;      // 在字符串中的位置
    int Len;
};

static char *CurrentInput;

typedef struct Node Node;
struct Node {
    NodeKind Kind; // 节点种类
    Node *LHS;     // 左部，left-hand side
    Node *RHS;     // 右部，right-hand side
    int Val;       // 存储ND_NUM种类的值
};

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

static bool startsWith(char *Str, char *SubStr) {
    return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

static int readPunct(char *Ptr) {
    if (    startsWith(Ptr, "==") 
        ||  startsWith(Ptr, "!=")
        ||  startsWith(Ptr, "<=")
        ||  startsWith(Ptr, ">="))
    {
        return 2;    
    }

    return ispunct(*Ptr) ? 1 : 0;
}

static int getNumber(Token *Tok) {
    if (Tok->Kind != TK_NUM)
        error("expect a number");
    return Tok->Val;
}

// 新建一个节点
static Node *newNode(NodeKind Kind) {
    Node *Nd = calloc(1, sizeof(Node));
    Nd->Kind = Kind;
    return Nd;
}
// 新建一个二叉树节点
static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS) {
    Node *Nd = newNode(Kind);
    Nd->LHS = LHS;
    Nd->RHS = RHS;
    return Nd;
}

// 新建一个单叉树
static Node *newUnary(NodeKind Kind, Node *Expr) {
    Node *Nd = newNode(Kind);
    Nd->LHS = Expr;
    return Nd;
}
// 新建一个数字节点
static Node *newNum(int Val) {
    Node *Nd = newNode(ND_NUM);
    Nd->Val = Val;
    return Nd;
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
    return Head.Next;
}


// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num
static Node *expr(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);
static Node *unary(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
// 解析加减
// expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **Rest, Token *Tok) {
    return equality(Rest, Tok);
}

static Node *equality(Token **Rest, Token *Tok) {
    Node *Nd = relational(&Tok, Tok);
    // ("+" mul | "-" mul)*
    while (true) {
        // "+" mul
        if (equal(Tok, "==")) {
            Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next));
            continue;
        }
        // "-" mul
        if (equal(Tok, "!=")) {
            Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next));
            continue;
        }
        *Rest = Tok;
        return Nd;
    }
}

// 解析比较关系
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **Rest, Token *Tok) {
    // add
    Node *Nd = add(&Tok, Tok);

    // ("<" add | "<=" add | ">" add | ">=" add)*
    while (true) {
        // "<" add
        if (equal(Tok, "<")) {
        Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next));
        continue;
        }

        // "<=" add
        if (equal(Tok, "<=")) {
        Nd = newBinary(ND_LE, Nd, add(&Tok, Tok->Next));
        continue;
        }

        // ">" add
        // X>Y等价于Y<X
        if (equal(Tok, ">")) {
        Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd);
        continue;
        }

        // ">=" add
        // X>=Y等价于Y<=X
        if (equal(Tok, ">=")) {
        Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd);
        continue;
        }

        *Rest = Tok;
        return Nd;
    }
}

// 解析加减
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **Rest, Token *Tok) {
    // mul
    Node *Nd = mul(&Tok, Tok);

    // ("+" mul | "-" mul)*
    while (true) {
        // "+" mul
        if (equal(Tok, "+")) {
        Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next));
        continue;
        }

        // "-" mul
        if (equal(Tok, "-")) {
        Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next));
        continue;
        }

        *Rest = Tok;
        return Nd;
    }
}

// 解析乘除
// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **Rest, Token *Tok) {
    // unary
    Node *Nd = unary(&Tok, Tok);
    while (true) {
        // "*" unary
        if (equal(Tok, "*")) {
        Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next));
        continue;
        }
        // "/" unary
        if (equal(Tok, "/")) {
        Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next));
        continue;
        }
        *Rest = Tok;
        return Nd;
    }
}

static Node *unary(Token **Rest, Token *Tok) {
    if (equal(Tok, "+")) {
        return unary(Rest, Tok->Next);
    }

    if (equal(Tok, "-")) {
        return newUnary(ND_NEG, unary(Rest, Tok->Next));
    }

    return primary(Rest, Tok);
}

// 解析括号、数字
// primary = "(" expr ")" | num
static Node *primary(Token **Rest, Token *Tok) {
    // "(" expr ")"
    if (equal(Tok, "(")) {
        Node *Nd = expr(&Tok, Tok->Next);
        *Rest = skip(Tok, ")");
        return Nd;
    }
    // num
    if (Tok->Kind == TK_NUM) {
        Node *Nd = newNum(Tok->Val);
        *Rest = Tok->Next;
        return Nd;
    }
    errorTok(Tok, "expected an expression");
    return NULL;
}
//
// 语义分析与代码生成
//
// 记录栈深度
static int Depth;
// 压栈，将结果临时压入栈中备用
// sp为栈指针，栈反向向下增长，64位下，8个字节为一个单位，所以sp-8
// 当前栈指针的地址就是sp，将a0的值压入栈
// 不使用寄存器存储的原因是因为需要存储的值的数量是变化的。
static void push(void) {
    printf("    addi sp, sp, -8\n");
    printf("    sd a0, 0(sp)\n");
    Depth++;
}
// 弹栈，将sp指向的地址的值，弹出到a1
static void pop(char *Reg) {
    printf("    ld %s, 0(sp)\n", Reg);
    printf("    addi sp, sp, 8\n");
    Depth--;
}
// 生成表达式
static void genExpr(Node *Nd) {
    // 加载数字到a0
    switch (Nd->Kind) {

    case ND_NUM:
        printf("    li a0, %d\n", Nd->Val);
        return;

    case ND_NEG:
        genExpr(Nd->LHS);
        printf("    neg a0, a0\n");
        return;
    
    default:
        break;
    }
    // 递归到最右节点
    genExpr(Nd->RHS);
    // 将结果压入栈
    push();
    // 递归到左节点
    genExpr(Nd->LHS);
    // 将结果弹栈到a1
    pop("a1");
    // 生成各个二叉树节点
    switch (Nd->Kind) {
    case ND_ADD: // + a0=a0+a1
        printf("    add a0, a0, a1\n");
        return;
    case ND_SUB: // - a0=a0-a1
        printf("    sub a0, a0, a1\n");
        return;
    case ND_MUL: // * a0=a0*a1
        printf("    mul a0, a0, a1\n");
        return;
    case ND_DIV: // / a0=a0/a1
        printf("    div a0, a0, a1\n");
        return;
    case ND_EQ:
    case ND_NE:
        // a0=a0^a1，异或指令
        printf("    xor a0, a0, a1\n");
        if (Nd->Kind == ND_EQ)
        // a0==a1
        // a0=a0^a1, sltiu a0, a0, 1
        // 等于0则置1
        printf("    seqz a0, a0\n");
        else
        // a0!=a1
        // a0=a0^a1, sltu a0, x0, a0
        // 不等于0则置1
        printf("    snez a0, a0\n");
        return;
    case ND_LT:
        printf("    slt a0, a0, a1\n");
        return;
    case ND_LE:
        // a0<=a1等价于
        // a0=a1<a0, a0=a1^1
        printf("    slt a0, a1, a0\n");
        printf("    xori a0, a0, 1\n");
        return;
    default:
        break;
    }
    error("invalid expression");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("%s: invalid number of arguments.", argv[0]);
    }

    CurrentInput = argv[1];
    Token *Tok = tokenize();

    Node *Node = expr(&Tok, Tok);

    if (Tok->Kind != TK_EOF)
        errorTok(Tok, "extra token");

    printf("    .global main\n");
    printf("main:\n");

    genExpr(Node);

    printf("    ret\n");

    assert(Depth == 0);

    return 0;
}