assert() {
    expected="$1"
    input="$2"

    ./rvcc $input > tmp.s || exit

    riscv64-unknown-linux-gnu-gcc -static tmp.s -o tmp

    # ./tmp
    qemu-riscv64 -L $RISCV/sysroot ./tmp

    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# assert 期待值 输入值
assert 0 0
assert 42 42

echo OK