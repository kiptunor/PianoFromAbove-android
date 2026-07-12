#ifndef UTILS_H
#define UTILS_H


#include <cstddef>
//#include <string>














namespace NVMidi /* ===== Tool function namespace ===== */
{
using nv_ul64 = unsigned long long;
using u32_t   = unsigned int;
using u16_t   = unsigned short;
using nv_byte = unsigned char;

using std::size_t;

/* Mapping strings to big-endian integers */
nv_ul64     operator""_u64be(const char *str, size_t n);


void        revU16(u16_t &x); // Reversing the end-order of a 16-bit integer variable
void        revU32(u32_t &x); // Reversing the end-order of a 32-bit integer variable
}; // namespace NVMidi
#endif
