# 1 "cilcode.tmp/ex4.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex4.c" 2
int main() {
  struct foo {
        int x; } foo;
  {
     struct foo {
        double d;
     };
     return foo.x;
  }
}
