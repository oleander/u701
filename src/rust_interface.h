extern "C"
{
  void transition_from_cpp(const uint8_t *event);
  void setup_rust();
  size_t ble_keyboard_write(uint8_t c);
  bool ble_keyboard_is_connected();
}
