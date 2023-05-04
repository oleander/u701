#if defined(INFO)
#define PRINT(...)   Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#define PRINTF(...)  Serial.printf(__VA_ARGS__)
#elif defined(IOS)
#define PRINT(...)                                                                                 \
  if (useKeyboardForLogging) keyboard.print(__VA_ARGS__);
#define PRINTLN(...)                                                                               \
  {                                                                                                \
    char _buf_ln[256];                                                                             \
    snprintf(_buf_ln, sizeof(_buf_ln), "%s\n", __VA_ARGS__);                                       \
    if (useKeyboardForLogging) keyboard.print(_buf_ln);                                            \
  }
#define PRINTF(...)                                                                                \
  {                                                                                                \
    char _buf[256];                                                                                \
    snprintf(_buf, sizeof(_buf), __VA_ARGS__);                                                     \
    if (useKeyboardForLogging) keyboard.print(_buf);                                               \
  }
#else
#define PRINT(...)
#define PRINTLN(...)
#define PRINTF(...)
#endif
