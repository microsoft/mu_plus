/**

Copyright (c) 2016, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#ifndef __MS_SAFE_BASE_LIB_H__
#define __MS_SAFE_BASE_LIB_H__

#define UINT32_MAX      0xffffffffui32

//
// It is common for -1 to be used as an error value
//
#define INT8_ERROR      (-1i8)
#define UINT8_ERROR     0xffui8
#define INT16_ERROR     (-1i16)
#define UINT16_ERROR    0xffffui16
#define INT_ERROR       (-1)
#define INT32_ERROR     (-1i32)
#define UINT_ERROR      0xffffffff
#define UINT32_ERROR    0xffffffffUL
#define INT64_ERROR     (-1i64)
#define UINT64_ERROR    0xffffffffffffffffULL

/**
 
  Safely adds two unsigned 32-bit numbers.

  @param  Augend  Supplies the 32-bit unsigned value to which the addend is to
                  be added.

  @param  Addend  Supplies the 32-bit unsigned value that is to be added to th
                  Augend.

  @param  Sum     Supplies a pointer to 32-bit value that will hold the result.
                  If the addition results in an overflow, the value copied into
                  this location will be UINT32_ERROR. If no overflow occurs,
                  value copied into this location will the sum of the addition
                  operation.

  @return TRUE if addition operation is successful. FALSE if the addition
          results in an overflow.

**/
BOOLEAN
SafeU32Add(
UINT32 Augend,
UINT32 Addend,
UINT32* Result
);

/**
 
  Safely adds two unsigned 64-bit numbers.

  @param  Augend  Supplies the 64-bit unsigned value to which the addend is to
                  be added.

  @param  Addend  Supplies the 64-bit unsigned value that is to be added to th
                  Augend.

  @param  Sum     Supplies a pointer to 64-bit value that will hold the result.
                  If the addition results in an overflow, the value copied into
                  this location will be UINT64_ERROR. If no overflow occurs,
                  value copied into this location will the sum of the addition
                  operation.

  @return TRUE if addition operation is successful. FALSE if the addition
          results in an overflow.

**/
BOOLEAN
SafeU64Add(
UINT64 Augend,
UINT64 Addend,
UINT64* Result
);

/**
 
 Safely converts an unsigned 64-bit integer to an unsigned 32-bit integer.

  @param  Value   Supplies the 64-bit unsigned value that is to be converted.

  @param  Result  Supplies a pointer to a 32-bit value that will hold the
                  result. If the supplied 64-bit value overflows 32-bits
                  UINT32_ERROR is returned in this location. Otherwise, the
                  converted value is returned in this this location.

  @return TRUE if addition operation is successful. FALSE if the conversion
          results in an overflow.

**/
BOOLEAN
U64ToU32(
UINT64 Value,
UINT32* Result
);
/**
 
 Safely multiplies two unsigned 32-bit integers.

  @param  Multiplicand   Supplies the 32-bit multiplicand.

  @param  Multiplier     Supplies the 32-bit multiplier.

  @param  Product        Supplies a pointer to a 32-bit value that will hold the
                         product. Upon return, this value will hold UINT32_ERROR
                         if the product overflows an unsigned 32-bit value.
                         Otherwise, it holds the product.

  @return TRUE if addition operation is successful. FALSE if the multiplication
          results in an overflow.

**/
BOOLEAN
SafeU32Mult(
UINT32 Multiplicand,
UINT32 Multiplier,
UINT32* Result
);
/**
 
 Safely multiplies two unsigned 64-bit integers.

  @param  Multiplicand   Supplies the 64-bit multiplicand.

  @param  Multiplier     Supplies the 64-bit multiplier.

  @param  Product        Supplies a pointer to a 64-bit value that will hold the
                         product. Upon return, this value will hold UINT64_ERROR
                         if the product overflows an unsigned 64-bit value.
                         Otherwise, it holds the product.

  @return TRUE if addition operation is successful. FALSE if the multiplication
          results in an overflow.

**/
BOOLEAN
SafeU64Mult(
UINT64 Multiplicand,
UINT64 Multiplier,
UINT64* Product
);

#endif // __MS_SAFE_BASE_LIB_H__
