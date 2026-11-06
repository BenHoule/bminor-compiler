struct pair {
  int first;
  int second;
};

int load_value(const int *address) {
  return *address;
}

void store_value(int *address, int value) {
  *address = value;
}

int array_element(const int *values, unsigned int index) {
  return values[index];
}

void array_store(int *values, unsigned int index, int value) {
  values[index] = value;
}

int pair_second(const struct pair *value) {
  return value->second;
}

int global_value = 7;

int load_global(void) {
  return global_value;
}
