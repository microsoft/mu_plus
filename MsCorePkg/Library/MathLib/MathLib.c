/** @file
MathLib

This library supports math operations such as Square Root, Cosine, and Sine

Copyright (c) 2018, Microsoft Corporation

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
#include <Uefi.h>
#include <Library/MathLib.h>

/**
Find sine of a provided double in radians

@param[in]  angle to calculate in radians

@retval the result (double)
**/
double
EFIAPI
sin_d(
  IN CONST double angleInRadians  
){
    double radians = angleInRadians;
    double previousValue = radians; //x0
    INT16 multiply = -1; //we subtract first possibly faster
    UINT32 iterationCount = 5; //
    double top; //x^3
    UINT64 denom = 3*2; //3!
    double value;
    //Using taylor series expansions
    //https://en.wikipedia.org/wiki/Trigonometric_functions#Series_definitions
    //so far seems comparable performance to the build in sin function from math.h
    //first range modify it to be within 0 and 2 pi
    while (radians > 2* MU_PI){
        radians -= 2*MU_PI;
    }
    while (radians < -2* MU_PI){
        radians += 2*MU_PI;
    }
    
    //compute the first iteration
    //Formula is sum over N to infinity with x being the radians
    // -1^n x^(2n+1)
    //----------------
    //    (2n+1)!
    
    top = radians * radians * radians; //x^3
    value = previousValue - (top/denom);

    //iterate 7 iterations
    for (;iterationCount <= 19;iterationCount+=2){ 
        previousValue = value;
        denom *= iterationCount * (iterationCount-1); //n * n-1 * (previous compued n-2!)
        top *= radians * radians; // x^2 * (previous computed x^n-2)
        multiply *= -1; //invert the sign
        //could be possibly faster to not use multiply but a conditional move?
        value = previousValue + (multiply*top/denom);
    }
    //checking for convergence provides neglieble speedup and a drop in accuracy.

    return value;
}


/**
Find cosine of a provided double in radians

@param[in]  angle to calculate in radians

@retval the result (double)
**/
double
EFIAPI
cos_d(
  IN CONST double angleInRadians
){
    double radians = angleInRadians;
    
    double previousValue = 1; //x0
    INT16 multiply = -1; //we subtract first possibly faster
    UINT32 iterationCount = 4; //we start at four
    double top;
    UINT64 denom = 2; //2!
    double value;
    //Using taylor series expansions
    //https://en.wikipedia.org/wiki/Trigonometric_functions#Series_definitions
    //so far seems to be slightly slower to built in COS
    //first range modify it to be within 0 and 2 pi
    while (radians > 2* MU_PI){
        radians -= 2*MU_PI;
    }
    while (radians < -2* MU_PI){
        radians += 2*MU_PI;
    }

    //compute the first iteration
    //Formula is sum over N to infinity with x being the radians
    // -1^n x^(2n)
    //----------------
    //    (2n)!
    
    top = radians * radians; //x^2
    value = previousValue - (top/denom);

    //iterate 7 iterations
    for (;iterationCount <= 20;iterationCount+=2){ 
        previousValue = value;
        denom *= iterationCount * (iterationCount-1); //n * n-1 * (previous compued n-2!)
        top *= radians * radians; // x^2 * (previous computed x^n-2)
        multiply *= -1; //invert the sign
        //could be possibly faster to not use multiply but a conditional move?
        value = previousValue + (multiply*top/denom);
    }
    //checking for convergence provides neglieble speedup and a drop in accuracy.
    return value;
}

//Bit scan reverse for 64 bit values
static inline UINT16 bsr64(UINT64 value) {
#if __GNUC__ > 3 //if we are using GCC take advantage of their builtins
    return 64 - __builtin_clzl(value);

#elif _MSC_VER > 1500 //if we are using MS VS compiler 15 or greater
    UINT64 result;
    #if defined _M_X64
        //https://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
        //this will only work on ARM and x64
		_BitScanReverse64(&result, value);
		return (UINT16) result + 1;	
    #else
       UINT32 value2 = value >> 32;
		//https://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
		// we split the operation up - first we check the upper bits and then check 
		if (_BitScanReverse(&result, value2)) {
			result += 32;
		}
		else {
			_BitScanReverse(&result, value);
		}

		return (UINT16)result + 1;

    #endif
#else //this is our fallback
	UINT16 count = 1;
	UINT16 result = value;
	if (value == 0) {
		return 0;
	}

	while (count < 64 && value != 0x1) {
		value = value >> 1;
		//printf("Checking %x at %d\n",value,count);
		count++;
	}
	return count;
#endif
}

// helper function to get the highest bit set and zeros everything else.
static inline UINT64 hibit(UINT64 n) {
    n |= (n >>  1);
    n |= (n >>  2);
    n |= (n >>  4);
    n |= (n >>  8);
    n |= (n >> 16);
    n |= (n >> 32);
    return n - (n >> 1);


}

/**
Find square root of the provided double
Currently not very fast -> needs setup
@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
double
EFIAPI
sqrt_d(
  IN CONST double input
){
    UINT64 firstGuess = (UINT64)input;
    double x = 0;
    double prevX = -1;

    //if we get anything under 0 or is zero return what we got
    if (input <= 0) return input;

    //find a reasonable first approximation for faster convergence
    // sqrt(input) = sqrt(a) * 2^n roughly equals 2^n
    // We find the highest order bit and xor everything else
    UINT64 highestOrderBit = hibit(firstGuess) /2;
    //then we get the position of the highest or'd bit
    //we might need to use another function other than clzl since that relies on BSR
    //this should output a bsrl on x86 and polyfill it in on ARM64
    UINT16 highestOrderBitPosition = bsr64(highestOrderBit);
    
    //our first guess is then just 2 ^ (n/2)
    firstGuess = (UINT64)1 << highestOrderBitPosition/2;

    //make sure our first guess is at least above zero
    if (firstGuess == 0) firstGuess = 1;

    //do a few iteration using Heron's method
    //https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
    x = (double)firstGuess;

    //do 7 iterations
    //any further iterations yields no accuracy benefits
    //quadratic convergent so we get 4x the precision each iteration 
    for (UINT64 i=0;i<6 && x != 0 && prevX != x;i++){
        prevX = x;
        x = .5 * (prevX + (input/prevX));
    }

    return x;
}


/**
Find square root of the provided unsigned integer

@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
UINT32
EFIAPI
sqrt32(
  IN CONST UINT32 input
){
    UINT32 res = 0;
    UINT32 bit = 1 << 30; // The second-to-top bit is set: 1 << 30 for 32 bits
    UINT32 num = input;
 
    // "bit" starts at the highest power of four <= the argument.
    while (bit > input)
        bit >>= 2;
        
    while (bit != 0) {
        //if num is more than result + bit
        if (num >= res + bit) {
            num -= res + bit;
            res += bit << 1;
        }
        
        res >>= 1;
        bit >>= 2;
    }
    return res;
}

/**
Find square root of the provided unsigned 64bit integer

@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
UINT64
EFIAPI
sqrt64(
  IN CONST UINT64 input
){
    UINT64 res = 0;
    UINT64 bit = (UINT64) 1 << 62; // The second-to-top bit is set: 1 << 62 for 64 bits
    UINT64 num = input;
 
    // "bit" starts at the highest power of four <= the argument.
    while (bit > input)
        bit >>= 2;
        
    while (bit != 0) {
        //if num is more than result + bit
        if (num >= res + bit) {
            num -= res + bit;
            res += bit << 1;
        }
        
        res >>= 1;
        bit >>= 2;
    }
    return res;
}

