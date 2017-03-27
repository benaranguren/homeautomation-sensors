// From https://github.com/bitluni/BasicRf/blob/master/RF.h

void rf_preamble(int pin, int time) {
  int start = micros();
  digitalWrite(pin, HIGH);
  while (micros() - start < time);
  digitalWrite(pin, LOW);
  while (micros() - start < time * 32);
}

void rf_write_bit(int pin, int time, int bit) {
  int start = micros();

  if (bit == 1) {
    // 3 highs, 1 low
    digitalWrite(pin, HIGH);
    while (micros() - start < time * 3);
    digitalWrite(pin, LOW);
    while (micros() - start < time * 4);
  } else {
    // 1 high, 3 lows
    digitalWrite(pin, HIGH);
    while (micros() - start < time * 1);
    digitalWrite(pin, LOW);
    while (micros() - start < time * 4);
  }
}

void rf_write_code(int pin, int time, int data, uint length) {
  rf_preamble(pin, time);

  for (int i = length - 1; i >= 0; i--) {
    rf_write_bit(pin, time, (data>>i) & 1);
  }
}
