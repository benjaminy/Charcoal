# 1 "cilcode.tmp/ex3.cil.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex3.cil.c" 2
# 1 "cilcode.tmp/ex3.c"
union baz {
   int x1 ;
   double x2 ;
};
# 1 "cilcode.tmp/ex3.c"
struct bar {
   union baz u1 ;
   int y ;
};
# 1 "cilcode.tmp/ex3.c"
struct foo {
   struct bar s1 ;
   int z ;
};
# 1 "cilcode.tmp/ex3.c"
struct foo f ;
