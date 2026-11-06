int increment(int value) {
  return value + 1;
}

int call_increment(int value) {
  int result = increment(value);
  return result + 2;
}

int sum_eight(int a, int b, int c, int d, int e, int f, int g, int h) {
  return a + b + c + d + e + f + g + h;
}

int call_sum_eight(int value) {
  return sum_eight(value, 2, 3, 4, 5, 6, 7, 8);
}
