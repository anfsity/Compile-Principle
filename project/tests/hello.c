int x;

int t() {
  x = x + 1;
  return 1;
}

int main() {
  int sum = 0;
  t();
  putint(x);
  return sum;
}