#ifdef __cplusplus
extern "C" {
#endif

typedef struct OneButton OneButton;

OneButton *one_button_new(int pin, bool activeLow, bool pullupActive);

// Declare other necessary functions from the OneButton class here

#ifdef __cplusplus
}
#endif