int sign_extend(signed char value) {
  return value;
}

unsigned int zero_extend(unsigned char value) {
  return value;
}

unsigned char truncate_integer(unsigned int value) {
  return (unsigned char)value;
}

double integer_to_double(int value) {
  return (double)value;
}

int double_to_integer(double value) {
  return (int)value;
}

double float_to_double(float value) {
  return value;
}

float double_to_float(double value) {
  return (float)value;
}
