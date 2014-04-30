# 1 "cilcode.tmp/ex7.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex7.c" 2
int main() {
  enum {
     FIVE = 5,
     SIX, SEVEN,
     FOUR = FIVE - 1,
     EIGHT = sizeof(double)
  } x = FIVE;
 return x;
}
