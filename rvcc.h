// 使用POSIX.1标准
// 使用了strndup函数
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    TK_IDENT, // 标记符，可以为变量名、函数名等
    TK_PUNCT,   // 操作符
    TK_NUM,
    TK_EOF,
    TK_KEYWORD
} TokenKind;

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整形
    ND_ADDR,      // 取地址 &
    ND_DEREF,     // 解引用 *
    ND_NEG,
    ND_ASSIGN,  // 赋值
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_EXPR_STMT,   // 表达式语句
    ND_VAR,      // 变量
    ND_RETURN,
    ND_IF,
    ND_FOR,
    ND_BLOCK,
    ND_FUNCALL,   // 函数调用
} NodeKind;

typedef struct Token Token;
struct Token {
    TokenKind Kind;
    Token *Next;
    int Val;
    char *Loc;      // 在字符串中的位置
    int Len;
};


void error(char *Fmt, ...);
void errorAt(char *Loc, char *Fmt, ...);
void errorTok(Token *Tok, char *Fmt, ...);
// 判断Token与Str的关系
bool equal(Token *Tok, char *Str);
Token *skip(Token *Tok, char *Str);
bool consume(Token **Rest, Token *Tok, char *Str);
// 词法分析
Token *tokenize(char *Input);

typedef struct Type Type;
typedef struct Node Node;
// 本地变量
typedef struct Obj Obj;
struct Obj {
    Obj *Next;  // 指向下一对象
    char *Name; // 变量名
    Type *Ty;
    bool IsLocal; // 是 局部或全局 变量
    int Offset; // fp的偏移量

    // 函数 或 全局变量
    bool IsFunction;

    // 函数
    Obj *Params;   // 形参
    Node *Body;    // 函数体
    Obj *Locals;   // 本地变量
    int StackSize; // 栈大小
};

struct Node {
    NodeKind Kind; // 节点种类
    Node *Next;     // 下一语句
    Token *Tok;    // 节点对应的终结符
    Type *Ty;      // 节点中数据的类型

    Node *LHS;     // 左部，left-hand side
    Node *RHS;     // 右部，right-hand side

    // "if"语句
    Node *Cond; // 条件内的表达式
    Node *Then; // 符合条件后的语句
    Node *Els;  // 不符合条件后的语句

    Node *Init; // 初始化
    Node *Inc;  // 递增

    Node *Body;

    // 函数调用
    char *FuncName; // 函数名
    Node *Args;     // 函数参数

    Obj *Var;
    int Val;       // 存储ND_NUM种类的值
};

// 语法解析入口函数
Obj *parse(Token *Tok);

//
// 类型系统
//
// 类型种类
typedef enum {
    TY_CHAR,  // char字符类型
    TY_INT, // int整型
    TY_PTR, // 指针
    TY_FUNC, // 函数
    TY_ARRAY, // 数组
} TypeKind;

struct Type {
    TypeKind Kind; // 种类
    int Size;      // 大小, sizeof返回的值
    Type *Base;    // 指向的类型
    Token *Name;
    // 数组
    int ArrayLen; // 数组长度, 元素总个数
    // 函数类型
    Type *ReturnTy; // 函数返回的类型
    Type *Params;   // 形参
    Type *Next;     // 下一类型
};

extern Type *TyChar;
// 声明一个全局变量，定义在type.c中。
extern Type *TyInt;

// 复制类型
Type *copyType(Type *Ty);
// 判断是否为整型
bool isInteger(Type *TY);
// 构建一个指针类型，并指向基类
Type *pointerTo(Type *Base);
// 为节点内的所有节点添加类型
void addType(Node *Nd);

// 数组类型
Type *arrayOf(Type *Base, int Size);

// 函数类型
Type *funcType(Type *ReturnTy);


//
// 语义分析与代码生成
//
// 代码生成入口函数
void codegen(Obj *Nd);