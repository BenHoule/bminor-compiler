int sum_to(int limit) {
  int total = 0;
  for (int i = 0; i < limit; i++) {
    total += i;
  }
  return total;
}

unsigned int countdown(unsigned int value) {
  unsigned int steps = 0;
  while (value != 0) {
    value--;
    steps++;
  }
  return steps;
}
