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


#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/MsSafeBaseLib.h>

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
)
{

  BOOLEAN Status;

  if ((Augend + Addend) >= Augend) {
    *Result = (Augend + Addend);
    Status = TRUE;

  }
  else {
    *Result = UINT32_ERROR;
    Status = FALSE;
  }

  return Status;
}

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
)
{

  BOOLEAN Status;

  if ((Augend + Addend) >= Augend) {
    *Result = (Augend + Addend);
    Status = TRUE;

  }
  else {
    *Result = UINT64_ERROR;
    Status = FALSE;
  }

  return Status;
}

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
)
{

  BOOLEAN Status;

  if (Value <= UINT32_MAX) {
    *Result = (UINT32)Value;
    Status = TRUE;

  }
  else {
    *Result = UINT32_ERROR;
    Status = FALSE;
  }

  return Status;
}

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
)
{

  UINT64 U64Product;

  U64Product = MultU64x64((UINT64)Multiplicand, (UINT64)Multiplier);
  return U64ToU32(U64Product, Result);
}

 
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
)
{

  BOOLEAN Result;

  // 64x64 into 128 is like 32.32 x 32.32.
  //
  // a.b * c.d = a*(c.d) + .b*(c.d) = a*c + a*.d + .b*c + .b*.d
  // back in non-decimal notation where A=a*2^32 and C=c*2^32:  
  // A*C + A*d + b*C + b*d
  // So there are four components to add together.
  //   result = (a*c*2^64) + (a*d*2^32) + (b*c*2^32) + (b*d)
  //
  // a * c must be 0 or there would be bits in the high 64-bits
  // a * d must be less than 2^32 or there would be bits in the high 64-bits
  // b * c must be less than 2^32 or there would be bits in the high 64-bits
  // then there must be no overflow of the resulting values summed up.

  UINT32 U32A;
  UINT32 U32B;
  UINT32 U32C;
  UINT32 U32D;
  UINT64 AD = 0;
  UINT64 BC = 0;
  UINT64 BD = 0;
  UINT64 U64Result = 0;

  Result = FALSE;

  U32A = (UINT32)(RShiftU64(Multiplicand,32));
  U32C = (UINT32)(RShiftU64(Multiplier,32));

  //
  // If high 32 bits of both values are zero, no chance for overflow.
  //
  if ((U32A == 0) && (U32C == 0)) {
    U32B = (UINT32)Multiplicand;
    U32D = (UINT32)Multiplier;
    *Product = MultU64x64(((UINT64)U32B), (UINT64)U32D);
    Result = TRUE;
    goto Cleanup;
  }

  //
  // a * c must be 0 or there would be bits set in the high 64-bits
  //
  if ((U32A != 0) && (U32C != 0)) {
    goto Cleanup;
  }

  //
  // a * d must be less than 2^32 or there would be bits set in the high 64-bits
  //
  U32D = (UINT32)Multiplier;
  AD = MultU64x64(((UINT64)U32A), (UINT64)U32D);
  if ((AD & 0xffffffff00000000) != 0) {
    goto Cleanup;
  }

  //
  // b * c must be less than 2^32 or there would be bits set in the high 64-bits
  //
  U32B = (UINT32)Multiplicand;
  BC = MultU64x64(((UINT64)U32B), (UINT64)U32C);
  if ((BC & 0xffffffff00000000) != 0) {
    goto Cleanup;
  }

  //
  // now sum them all up checking for overflow.
  // shifting is safe because we already checked for overflow above
  //
  if (SafeU64Add(LShiftU64(BC, 32), LShiftU64(AD,32), &U64Result) == FALSE) {
    goto Cleanup;
  }

  //
  // b * d
  //
  BD = MultU64x64(((UINT64)U32B), (UINT64)U32D);
  if (SafeU64Add(U64Result, BD, &U64Result) == FALSE) {
    goto Cleanup;
  }

  Result = TRUE;
  *Product = U64Result;

Cleanup:
  if (Result == FALSE) {
    *Product = UINT64_ERROR;
  }

  return Result;
}

