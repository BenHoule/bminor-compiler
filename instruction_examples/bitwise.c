unsigned int bit_and(unsigned int a, unsigned int b) {
  return a & b;
}

unsigned int bit_or(unsigned int a, unsigned int b) {
  return a | b;
}

unsigned int bit_xor(unsigned int a, unsigned int b) {
  return a ^ b;
}

unsigned int bit_not(unsigned int value) {
  return ~value;
}

unsigned int shift_left(unsigned int value, unsigned int count) {
  return value << count;
}

unsigned int logical_shift_right(unsigned int value, unsigned int count) {
  return value >> count;
}

int arithmetic_shift_right(int value, unsigned int count) {
  return value >> count;
}
