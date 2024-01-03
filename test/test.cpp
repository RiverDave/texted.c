#include <iostream>

int sum(const int &x, const int &y);

int main (int argc, char *argv[]) {
  
  std::cout << sum(23,23) <<std::endl;
  
  return 0;
}

int sum(int &x, int &y){
  return x + y;
}

