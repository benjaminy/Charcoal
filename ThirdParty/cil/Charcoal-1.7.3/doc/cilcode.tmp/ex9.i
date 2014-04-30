# 1 "cilcode.tmp/ex9.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex9.c" 2
  struct foo {
     int x, y;
     int a[5];
     struct inner {
        int z;
     } inner;
  } s = { 0, .inner.z = 3, .a[1 ... 2] = 5, 4, y : 8 };
