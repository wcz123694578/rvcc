#!/bin/bash

# 将下列代码编译为tmp2.o，"-xc"强制以c语言进行编译
# cat <<EOF | $RISCV/bin/riscv64-unknown-linux-gnu-gcc -xc -c -o tmp2.o -
cat <<EOF | $RISCV/bin/riscv64-unknown-linux-gnu-gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

# 声明一个函数
assert() {
  # 程序运行的 期待值 为参数1
  expected="$1"
  # 输入值 为参数2
  input="$2"

  # 运行程序，传入期待值，将生成结果写入tmp.s汇编文件。
  # 如果运行不成功，则会执行exit退出。成功时会短路exit操作
  ./rvcc "$input" > tmp.s || exit
  # 编译rvcc产生的汇编文件
  # gcc -o tmp tmp.s
  $RISCV/bin/riscv64-unknown-linux-gnu-gcc -static -o tmp tmp.s tmp2.o

  # 运行生成出来目标文件
  ./tmp
  # $RISCV/bin/qemu-riscv64 -L $RISCV/sysroot ./tmp
  # $RISCV/bin/spike --isa=rv64gc $RISCV/riscv64-unknown-linux-gnu/bin/pk ./tmp

  # 获取程序返回值，存入 实际值
  actual="$?"

  # 判断实际值，是否为预期值
  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# assert 期待值 输入值
# # [1] 返回指定数值
# assert 0 '0;'
# assert 42 '42;'

# # [2] 支持+ -运算符
# assert 34 '12-34+56;'

# # [3] 支持空格
# assert 41 ' 12 + 34 - 5 ;'

# # [4] 支持* /
# assert 116 '30 * 4 - 20 / 5;'
# assert 15 '5*(9-6);'
# assert 17 '1-8/(2*2)+3*6;'

# # [5] 支持一元运算的+ -
# assert 10 '-10+20;'
# assert 10 '- -10;'
# assert 10 '- - +10;'
# assert 48 '------12*+++++----++++++++++4;'

# # [6] 支持关系运算符
# assert 0 '0==1;'
# assert 1 '42==42;'
# assert 1 '0!=1;'
# assert 0 '42!=42;'
# assert 1 '0<1;'
# assert 0 '1<1;'
# assert 0 '2<1;'
# assert 1 '0<=1;'
# assert 1 '1<=1;'
# assert 0 '2<=1;'
# assert 1 '1>0;'
# assert 0 '1>1;'
# assert 0 '1>2;'
# assert 1 '1>=0;'
# assert 1 '1>=1;'
# assert 0 '1>=2;'
# assert 1 '5==2+3;'
# assert 0 '6==4+3;'
# assert 1 '0*9+5*2==4+4*(6/3)-2;'

# # [7] 支持分号
# assert 1 '2+1;3!=4;'

# # [8] 支持单字母本地变量
# assert 4 'a = b = 2; a + b;'

# # [9] 支持多字母变量
# assert 3 'foo=3; foo;'
# assert 74 'foo2=70; bar4=4; foo2+bar4;'

# # [10] return
# assert 7 '{ var1 = 3; var2 = 4; return var1 + var2; }'

# # [11] 空语句
# assert 5 '{ ;;;; return 5; }'

# # [12] 支持if语句
# assert 3 '{ a=3;b=2; if (a==b) return 2; return 3; }'
# assert 2 '{ if (1==1) return 2; return 3; }'
# assert 2 '{ if (1) return 2; return 3; }'
# assert 2 '{ if (2-1) return 2; return 3; }'
# assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
# assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

# # [13] 支持for语句
# assert 55 '{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
# assert 3 '{ for (;;) {return 3;} return 5; }'

# # [14] 支持while语句
# assert 10 '{ i=0; while(i<10) { i=i+1; } return i; }'

# # [15] 支持一元& *运算符
# assert 3 '{ int x=3; return *&x; }'
# assert 3 '{ int x=3; int *y=&x; int **z=&y; return **z; }'
# assert 5 '{ int x=3; int *y=&x; *y=5; return x; }'

# # [16] 支持指针的算术运算
# assert 3 '{ int x=3; int y=5; return *(&y-1); }'
# assert 5 '{ int x=3; int y=5; return *(&x+1); }'
# assert 7 '{ int x=3; int y=5; *(&y-1)=7; return x; }'
# assert 7 '{ int x=3; int y=5; *(&x+1)=7; return y; }'

# # [17] 支持零参函数调用
# assert 3 '{ return ret3(); }'
# assert 5 '{ return ret5(); }'
# assert 8 '{ return ret3()+ret5(); }'

# # [24] 支持最多6个参数的函数调用
# assert 8 'int main() { return add(3, 5); }'
# assert 2 'int main() { return sub(5, 3); }'
# assert 21 'int main() { return add6(1,2,3,4,5,6); }'
# assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
# assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

# # [25] 支持最多6个参数的函数定义
# assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
# assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
# assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

# # [27] 支持一维数组
# assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'
# assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
# assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
# assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

# # [28] 支持多维数组
# assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
# assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
# assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
# assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
# assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
# assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'

# # [29] 支持 [] 操作符
# assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
# assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
# assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
# assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
# assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'
# assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
# assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
# assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
# assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
# assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
# assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'

# # [30] sizeof
# assert 8 'int main() { int a; return sizeof(a); }'
# assert 8 'int main() { int a; return sizeof a; }'

# # [31] 全局变量
# assert 3 'int a; int main() { a = 3; return a; }'

# [33] 支持char类型
assert 1 'int main() { char x=1; return x; }'
assert 1 'int main() { char x=1; char y=2; return x; }'
assert 2 'int main() { char x=1; char y=2; return y; }'
assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'


# 如果运行正常未提前退出，程序将显示OK
echo OK