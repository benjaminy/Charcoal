# 1 "cilcode.tmp/ex7.cil.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex7.cil.c" 2
# 2 "cilcode.tmp/ex7.c"
enum __anonenum_x_1 {
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    FOUR = 4,
    EIGHT = 8
} ;
# 1 "cilcode.tmp/ex7.c"
int main(void)
{
  enum __anonenum_x_1 x ;

  {
# 2 "cilcode.tmp/ex7.c"
 x = (enum __anonenum_x_1 )5;
# 8 "cilcode.tmp/ex7.c"
 return ((int )x);
}
}
