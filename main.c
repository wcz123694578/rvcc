#include "rvcc.h"






// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num
//


int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("%s: invalid number of arguments.", argv[0]);
    }

    Token *Tok = tokenize(argv[1]);

    Obj *Prog = parse(Tok);

    codegen(Prog);

    return 0;
}