# 1 "cilcode.tmp/ex21.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex21.c" 2
int main(void) {
# 1 "cilcode.tmp/ex21.c"
   int x, y, z;
   return &(x ? y : z) - & (x ++, x);
}
