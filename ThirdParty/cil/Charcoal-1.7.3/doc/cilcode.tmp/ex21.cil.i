# 1 "cilcode.tmp/ex21.cil.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex21.cil.c" 2
# 1 "cilcode.tmp/ex21.c"
int main(void)
{
  int x ;
  int y ;
  int z ;
  int *tmp ;

  {
# 2 "cilcode.tmp/ex21.c"
 if (x) {
# 2 "cilcode.tmp/ex21.c"
 tmp = & y;
  } else {
# 2 "cilcode.tmp/ex21.c"
 tmp = & z;
  }
# 2 "cilcode.tmp/ex21.c"
 x ++;
# 2 "cilcode.tmp/ex21.c"
 return ((int )(tmp - & x));
}
}
