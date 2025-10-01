#include <stdio.h>
#include "tf32.h"

/*
    Do NOT include any C libraries or header files
    except those above
    
    use the defines in tf32.h
    if neccessary, you can add some macros below
*/

tf32 int2tf32(int in) {
    tf32 result = 0;

    if (in == 0){
        return result;  // zero
    }

    // sign bit
    int sign = (in > 0) ? 0 : 1;

    // magnitude in decimal
    int mnt = (sign == 0) ? in : -in;

    // mantissa and exp
    int mantissa = 0;
    int exp = 0;

    for (int i = 30; i >= 0; i--)   // starts from 30 s.t. sign bit is ignored
    {
        mantissa = mantissa << 1;   // make room for next bit
        mantissa |= (mnt >> i) & 1;     // extracts i-th bit

        if (exp == 0 && (mnt >> i & 1)){   //if exp is zero and bit index is nonzero
            exp = i;
        }
    }

    // add bias
    exp += 127;

    /*
    How to view mantissa as binary value
    for (int i = 30; i >= 0; i--) {
        printf("%d", (mantissa >> i) & 1);
    }
    printf("\n");
    */

    // build f32 number
    // write sign bit
    result |= (sign << 18);

    // write exp
    for (int i = 7; i >= 0; i--)
    {
        result |= ((exp >> i) & 1) << (10 + i);
    }

    // mantissa (print "true exp - 1" places)
    for (int i = exp - 128; i >= 0; i--)
    {
        result |= ((mantissa >> i) & 1) << (9 - exp + 128 + i);
    }

    /*
    put_tf32(result);   // prints result
    printf("\n");
    */

    return result;
}


int tf322int(tf32 in) {
    int result = 0;

    // Extract sign bit
    int sign = (in >> 18) & 1;

    // extract exp
    int exp = 0;

    for (int i = 7; i >= 0; i--)
    {
        exp |= (((in >> (i + 10)) & 1) << i);
    }

    // extract mantissa
    int mantissa = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa |= ((in >> i) & 1) << i; // adjust
    }
    
    // 1. Positive or negative infinity
    if (exp == 255 && mantissa == 0)
    {
        int tmax = (1 << 31) - 1;
        return tmax;
    }

    // 2. Greater than TMax
    if ((exp - 127) > 30 && sign == 0)
    {
        int tmax = (1 << 31) - 1;
        return tmax;
    }

    // 3. Less than TMin
    if (sign == 1 && ((exp - 127) > 31 || ((exp -127) > 30 && mantissa > 0)))
    {
        int tmin = (1 << 31);
        return tmin;
    }

    // 4. Positive or Negative NaN
    if (exp == 255 && mantissa != 0)
    {
        int tmin = (1 << 31);
        return tmin;
    }

    // 5. Positive or Negative zero
    if (exp == 0 && mantissa == 0)
    {
        return 0;
    }


    // END OF OTHER OUTCOMES

    if (exp != 0)
    {
        mantissa |= 1 << 10;    //add 1 for normalized value
    }



    // 6. Actual integer
    // Convert mantissa to int number
    int E = exp - 127; // unbiased exponent

    if (E < 0)  // guaranteed to round to 0
    {
        result = 0;
    }
    else if (E > 10){   // since mantissa has 11 places by default (including 1)
        result = mantissa << (E - 10);
    }
    else{
        int shift = 10 - E;           // how many bits to discard
        int base = mantissa >> shift; // truncated result

        int guard = (mantissa >> (shift - 1)) & 1;  // the bit right below cutoff
        int rest  = mantissa & ((1 << (shift - 1)) - 1); // all bits below guard
        int lsb   = base & 1; // least significant bit of truncated result

        // Round to nearest-even
        if (guard && (rest || lsb)) {
            base++;
        }

        result = base;
    }

    /*
    ROUNDING
    */


    if (sign)
    {
        result = -result;
    }

    return result;
}


tf32 double2tf32(double in) {
    tf32 result = 0;

    union DoubleBits{ double f; unsigned long long u; } x;
    x.f = in;

    // sign bit, exp, mantissa
    unsigned long long sign = x.u & 0x8000000000000000ULL;
    unsigned long long exponent = (x.u >> 52) & 0x7FF;       // 11 bits
    unsigned long long mantissa =  x.u & 0xFFFFFFFFFFFFFULL; // 52 bits


    // insert sign bit
    result |= ((sign >> 63) << 18);

    // detect 0
    if (in == 0) {
        return result;
    }

    // detect NaN
    if (exponent == 2047 && mantissa != 0)
    {
        result |= (0xFF << 10);
        result |= 1;

        return result;
    }

    // change bias
    exponent -= 1023;
    exponent += 127;

    // if double overflows, return positive or negative infinity
    if (exponent >= 255)
    {
        result |= (0xFF << 10);

        return result;
    }


    // build f32 number
    // write exp
    for (int i = 7; i >= 0; i--)
    {
        result |= ((exponent >> i) & 1) << (10 + i);    // ignores the upper bits
    }

    // mantissa (print "true exp - 1" places)
    // check the truncated bits, and round if necessary
    unsigned int mant10 = mantissa >> (52 - 10);
    unsigned long long mant42 = mantissa & ((1ULL << (52 - 10)) - 1);
    unsigned long long halfway   = 1ULL << (52 - 10 - 1);
    
    if (mant42 > halfway)
    {
        mant10++;
    }
    else if (mant42 == halfway && (mant10 & 1)) {   // round to even
        mant10++;
    }

    //edge case: mantissa overflowing after rounding
    if (mant10 == (1 << 10)) {
        mant10 = 0;
        exponent++;

        if (exponent >= 255) {
            result &= ~(0x3FFFF);   // clear exp+mant
            result |= (0xFF << 10); // infinity
            return result;
        }
    }

    result |= mant10;    

    return result;
}

double tf322double(tf32 in) {
    double result = 0.0;

    // Extract sign bit
    int sign = (in >> 18) & 1;

    // extract exp
    int exp = 0;

    for (int i = 7; i >= 0; i--)
    {
        exp |= (((in >> (i + 10)) & 1) << i);
    }

    // extract mantissa
    int mantissa = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa |= ((in >> i) & 1) << i; // adjust
    }


    
    // 1. Positive or negative infinity
    if (exp == 255 && mantissa == 0)
    {
        // infinity value for double, exp = max and mantissa = 0
        unsigned long long dres = ((unsigned long long)sign << 63) | (0x7FFULL << 52); 
        
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }

    // 2. Positive or Negative NaN
    if (exp == 255 && mantissa != 0)
    {
        unsigned long long double_mant = (unsigned long long) mantissa << (52 - 10);
        unsigned long long dres = ((unsigned long long)sign << 63) | (0x7FFULL << 52) | double_mant; 
        
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }

    // 3. Positive or Negative zero
    if (exp == 0 && mantissa == 0)
    {
        union { unsigned long long u; double f; } u;
        u.u = 0;
        u.u |= (unsigned long long) sign << 63;
        return u.f;
    }

    /*
    // 4. Positive or negative subnormal
    // NOT NECESSARY since subnormal numbers for TF32 is not subnormal for double
    if (exp == 0 && mantissa != 0)
    {
        unsigned long long mant = (unsigned long long) mantissa << (52 - 10);
        unsigned long long dres = ((unsigned long long)sign << 63) | mant; 
        
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }
    */



    // END OF OTHER OUTCOMES
    // 5. Actual double
    // Convert exp to double exp
    exp = exp + 1023 - 127;


    // Convert to double value
    union { unsigned long long u; double f; } u;
    u.u = 0;
    u.u |= ((unsigned long long) sign << 63) | ((unsigned long long) exp << 52) | ((unsigned long long) mantissa << (52 - 10));

    result = u.f;

    // NO ROUNDING REQUIRED

    return result;
}

tf32 tf32_add(tf32 a, tf32 b) {
    tf32 result = 0;

    // Extract sign bits
    int sign1 = (a >> 18) & 1;
    int sign2 = (b >> 18) & 1;

    // extract exps
    int exp1 = 0;
    int exp2 = 0;

    for (int i = 7; i >= 0; i--)
    {
        exp1 |= (((a >> (i + 10)) & 1) << i);
        exp2 |= (((b >> (i + 10)) & 1) << i);
    }

    // extract mantissas
    int mantissa1 = 0;
    int mantissa2 = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa1 |= ((a >> i) & 1) << i; // adjust
        mantissa2 |= ((b >> i) & 1) << i;
    }

    // NaN or infinity check

    // if either value is NaN, return an arbitrary NaN
    if (exp1 == 255 && mantissa1 != 0)
    {
        unsigned long long dres = ((unsigned long long) sign1 << 63) | (0x7FFULL << 52) | 1; 
        
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }

    // when number 2 is NaN
    if (exp2 == 255 && mantissa2 != 0){
        unsigned long long dres = ((unsigned long long) sign2 << 63) | (0x7FFULL << 52) | 1; 
        
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }

    // infinity check (both)
    if (((exp1 == 255 && mantissa1 == 0) && (exp2 == 255 && mantissa2 == 0)))
    {
        // if positive infty + negative infty, return positive NaN
        if ((sign1 != sign2)) {
            unsigned long long dres = ((unsigned long long) sign1 << 63) | (0x7FFULL << 52) | 1; 
            
            union { unsigned long long u; double f; } u;
            u.u = dres;

            return u.f;
        }
        else {  // same sign, return infinity with the sign kept
            unsigned long long dres = ((unsigned long long) sign1 << 63) | (0x7FFULL << 52); 
            
            union { unsigned long long u; double f; } u;
            u.u = dres;

            return u.f;
        }
    }

    // only one is infinity
    if (exp1 == 255 && mantissa1 == 0){
        // return infinity with retained sign
        unsigned long long dres = ((unsigned long long) sign1 << 63) | (0x7FFULL << 52); 
            
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }

    if (exp2 == 255 && mantissa2 == 0){
        // return infinity with retained sign
        unsigned long long dres = ((unsigned long long) sign2 << 63) | (0x7FFULL << 52); 
            
        union { unsigned long long u; double f; } u;
        u.u = dres;

        return u.f;
    }


    // NaN/infinity check is over


    // adding up
    // compare exps
    if (exp1 > exp2) {
        int shift = exp1 - exp2;

        // consider implicit leading 1
        manA = mantissa1 | (1 << 10);
        manB = lbits | (1 << 10);

        // removed bits and leftover bits
        int rbits = manB & ((1 << shift) - 1);
        int manB = manB >> shift;

        // compute mantissa sum

        int smantissa = manA + manB;

        // if 
        if (smantissa >= (1 << 11))
        

    }
    else if (exp1 < exp2){

    }
    else{   // same value

    }


    // check if it overflows



    return result;
}

tf32 tf32_mul(tf32 a, tf32 b) {
    tf32 result = 0;
    /*
    fill this function
    */
    return result;
}

tf32 tf32_div(tf32 a, tf32 b) {
    tf32     result = 0;
    /*
    fill this function
    */
    return result;
}

int tf32_compare(tf32 a, tf32 b) {
    int result = 0;
    /*
    fill this function
    */
    return result;
}
