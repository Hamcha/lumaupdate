/* Bra.c -- Converters for RISC code
2010-04-16 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "Bra.h"

SizeT ARM_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 4)
    return 0;
  size -= 4;
  ip += 8;
  for (i = 0; i <= size; i += 4)
  {
    if (data[i + 3] == 0xEB)
    {
      UInt32 dest;
      UInt32 src = ((UInt32)data[i + 2] << 16) | ((UInt32)data[i + 1] << 8) | (data[i + 0]);
      src <<= 2;
      if (encoding)
        dest = ip + (UInt32)i + src;
      else
        dest = src - (ip + (UInt32)i);
      dest >>= 2;
      data[i + 2] = (Byte)(dest >> 16);
      data[i + 1] = (Byte)(dest >> 8);
      data[i + 0] = (Byte)dest;
    }
  }
  return i;
}

SizeT ARMT_Convert(Byte *data, SizeT size, UInt32 ip, int encoding)
{
  SizeT i;
  if (size < 4)
    return 0;
  size -= 4;
  ip += 4;
  for (i = 0; i <= size; i += 2)
  {
    if ((data[i + 1] & 0xF8) == 0xF0 &&
        (data[i + 3] & 0xF8) == 0xF8)
    {
      UInt32 dest;
      UInt32 src =
        (((UInt32)data[i + 1] & 0x7) << 19) |
        ((UInt32)data[i + 0] << 11) |
        (((UInt32)data[i + 3] & 0x7) << 8) |
        (data[i + 2]);
      
      src <<= 1;
      if (encoding)
        dest = ip + (UInt32)i + src;
      else
        dest = src - (ip + (UInt32)i);
      dest >>= 1;
      
      data[i + 1] = (Byte)(0xF0 | ((dest >> 19) & 0x7));
      data[i + 0] = (Byte)(dest >> 11);
      data[i + 3] = (Byte)(0xF8 | ((dest >> 8) & 0x7));
      data[i + 2] = (Byte)dest;
      i += 2;
    }
  }
  return i;
}