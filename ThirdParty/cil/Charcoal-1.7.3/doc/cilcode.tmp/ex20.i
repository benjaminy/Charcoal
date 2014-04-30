# 1 "cilcode.tmp/ex20.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex20.c" 2
int main(void) {
# 1 "cilcode.tmp/ex20.c"
   int x = 5, y = x;
   int z = ({ x++; L: y -= x; y;});
   return ({ goto L; 0; });
}
