# 1 "cilcode.tmp/ex19.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 170 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "cilcode.tmp/ex19.c" 2
int main(void) {
# 1 "cilcode.tmp/ex19.c"
   int x, y;
   for(int i = 0; i<5; i++) {
      if(i == 5) continue;
      if(i == 4) break;
      i += 2;
   }
   while(x < 5) {
     if(x == 3) continue;
     x ++;
   }
}
