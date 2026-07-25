#include "utf8_helper.h"

size_t Calculate_UTF8_Max_Size(unsigned int amount)
{
    // The largest size of a UTF8 character is 4 bytes
    return amount * 4;
}

bool Get_Bit_At(unsigned char byte, unsigned int position)
{
    return (byte & (1 << 7 - position)) != 0;
}

bool UTF8_Is_Start_Char(char character)
{
    return Get_Bit_At(character, 0) == false || Get_Bit_At(character, 1) == true;
}