# 1 "cilcode.tmp/ex4.cil.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex4.cil.c" 2
# 2 "cilcode.tmp/ex4.c"
struct foo {
   int x ;
};
# 1 "cilcode.tmp/ex4.c"
int main(void)
{
  struct foo foo ;

  {
# 8 "cilcode.tmp/ex4.c"
 return (foo.x);
}
}
