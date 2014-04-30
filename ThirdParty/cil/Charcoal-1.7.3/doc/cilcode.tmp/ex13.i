# 1 "cilcode.tmp/ex13.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex13.c" 2
  int x = 5;
  int main() {
    int x = 6;
    {
      int x = 7;
      return x;
    }
    return x;
  }
