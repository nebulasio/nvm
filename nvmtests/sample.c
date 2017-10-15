
#include <stdio.h>


void func_a() { printf("called to func_a.\n"); }

void func_b() {
  int v = 0;
  printf("called to func_b, dice is %d.\n", v);
}

void func_c_with_param(int a){
  printf("called to func_c_with_param: %d.\n", a);
}

int func_d_with_ret(){
  printf("called to func_d_with_ret.\n");
  return 0;
}

void nebulas_main(){
  printf("called to main.\n");
  func_a();
  func_b();
}

