#include "utils.h"







NVMidi::nv_ul64 NVMidi::operator"" _u64be(const char *str, size_t n)
{
    nv_ul64 ans = 0;
    u16_t   sft = 0; 

    while(nv_ul64 ch = str[sft])
    {
        ans |= ch << (sft++ << 3);
    }

    return ans;
}

void NVMidi::revU16(NVMidi::u16_t &x)
{
    x = x >> 8 | x << 8;
}

void NVMidi::revU32(NVMidi::u32_t &x)
{
    x = x >> 24 | (x & 0xFF0000) >> 8 | (x & 0xFF00) << 8 | x << 24;
}