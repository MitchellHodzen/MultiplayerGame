#ifndef UTF8_HELPER_DEF
#define UTF8_HELPER_DEF
#include <stdbool.h>
#include <stdlib.h>

size_t Calculate_UTF8_Max_Size(unsigned int amount);
bool UTF8_Is_Start_Char(char character);

#endif /* UTF8_HELPER_DEF */