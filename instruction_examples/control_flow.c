int choose(int condition, int when_true, int when_false) {
  if (condition) {
    return when_true;
  } else {
    return when_false;
  }
}

int logical_and(int a, int b) {
  return a && b;
}

int logical_or(int a, int b) {
  return a || b;
}

int logical_not(int value) {
  return !value;
}

int conditional_value(int condition, int a, int b) {
  return condition ? a : b;
}
