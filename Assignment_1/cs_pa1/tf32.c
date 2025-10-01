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

    exp = (in >> 10) & 0xFF;

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

    exp1 = (a >> 10) & 0xFF;
    exp2 = (b >> 10) & 0xFF;

    // extract mantissas
    int mantissa1 = 0;
    int mantissa2 = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa1 |= ((a >> i) & 1) << i; // adjust
        mantissa2 |= ((b >> i) & 1) << i;
    }

    // check if subtracting same number
    if (sign1 != sign2 && exp1 == exp2 && mantissa1 == mantissa2)
    {
        return result;
    }

    // NaN or infinity check

    // if either value is NaN, return an arbitrary NaN
    if (exp1 == 255 && mantissa1 != 0)
    {
        result = (sign1 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // when number 2 is NaN
    if (exp2 == 255 && mantissa2 != 0){
        result = (sign2 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // infinity check (both)
    if (((exp1 == 255 && mantissa1 == 0) && (exp2 == 255 && mantissa2 == 0)))
    {
        // if positive infty + negative infty, return positive NaN
        if ((sign1 != sign2)) {
            result = (0 << 18) | (0xFF << 10) | 1;

            return result;
        }
        else {  // same sign, return infinity with the sign kept
            result = (sign1 << 18) | (0xFF << 10);

            return result;
        }
    }

    // only one is infinity
    if (exp1 == 255 && mantissa1 == 0){
        // return infinity with retained sign
        result = (sign1 << 18) | (0xFF << 10);

        return result;
    }

    if (exp2 == 255 && mantissa2 == 0){
        // return infinity with retained sign
        result = (sign2 << 18) | (0xFF << 10);

        return result;
    }


    // NaN/infinity check is over


    // adding up
    // compare exps, expA is the larger one
    int signA, signB, expA, expB, manA, manB;

    if (exp1 > exp2) {
        signA = sign1;
        signB = sign2;

        expA = exp1;
        expB = exp2;

        // consider implicit leading 1, consider subnormal values
        manA = (exp1 ? ((1 << 10) | mantissa1) : mantissa1);
        manB = (exp2 ? ((1 << 10) | mantissa2) : mantissa2);
    }
    else { // less than or equal to
        signA = sign2;
        signB = sign1;

        expA = exp2;
        expB = exp1;

        // consider implicit leading 1
        manA = (exp2 ? ((1 << 10) | mantissa2) : mantissa2);
        manB = (exp1 ? ((1 << 10) | mantissa1) : mantissa1);
    }

    int shift = expA - expB;

    // removed bits and leftover bits
    int rbits = manB & ((shift >= 32 ? -1 : (1 << shift) - 1));
    int lbits = manB >> shift;
    int smantissa;


    // compute mantissa sum or difference
    if (signA == signB){ // sum
        smantissa = manA + lbits;
    }
    else {  // difference
        smantissa = manA - lbits;
    }

    // if mantissa overflows, change exponent
    if (smantissa >= (1 << 11)){
        smantissa >>= 1;
        expA++;

        // if overflows to infinity
        if (expA >= 255) {  
            result = (signA << 18) | (0xFF << 10); 
            
            return result;
        }
    }

    // if mantissa underflows, change exponent
    while (smantissa > 0 && smantissa < (1 << 10) && expA > 0){
        smantissa <<= 1;
        expA--;
    }

    // if subnormal, just calculate as usual

    int mant_out = smantissa & ((1 << 10) - 1); // keep only 10 LSBs (remove implied 1)
    int guard  = smantissa & 1;
    int sticky = (rbits != 0);
    if (guard && (sticky || (mant_out & 1))) {
        mant_out++;
        if (mant_out == (1 << 10)) {
            mant_out = 0;
            expA++;
        }
    }

    // reconstruct number and return
    result = (signA << 18) | (expA << 10) | (mant_out);

    return result;
}

tf32 tf32_mul(tf32 a, tf32 b) {
    tf32 result = 0;

     // Extract sign bits
    int sign1 = (a >> 18) & 1;
    int sign2 = (b >> 18) & 1;

    // extract exps
    int exp1 = 0;
    int exp2 = 0;

    exp1 = (a >> 10) & 0xFF;
    exp2 = (b >> 10) & 0xFF;


    // extract mantissas
    int mantissa1 = 0;
    int mantissa2 = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa1 |= ((a >> i) & 1) << i; // adjust
        mantissa2 |= ((b >> i) & 1) << i;
    }


    // NaN and infinity check
    // if one of them is NaN, return NaN
    if (exp1 == 255 && mantissa1 != 0)
    {
        result = (sign1 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // when number 2 is NaN
    if (exp2 == 255 && mantissa2 != 0){
        result = (sign2 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // infinity check (both)
    if (((exp1 == 255 && mantissa1 == 0) || (exp2 == 255 && mantissa2 == 0)))
    {
        // if multiplying zero and infinity, return NaN
        if ((exp1 == 0 && mantissa1 == 0) || (exp2 == 0 && mantissa2 == 0))
        {
            // +/- NaN
            result = ((sign1 ^ sign2) << 18) | (0xFF << 10) | 1;

            return result;
        }

        // when multiplying with normal value or other infinity, if sign is different, return negative infinity, otherwise return positive infinity
        result = ((sign1 ^ sign2) << 18) | (0xFF << 10);

        return result;
    }


    


    // End of NaN and infinity check
    // implicit leading 1, handle subnormal value
    if (exp1 == 0) {
        exp1 = -126; 
    } 
    else {
        mantissa1 |= (1 << 10);
        exp1 = exp1 - 127; // remove bias
    }

    if (exp2 == 0) {
        exp2 = -126;
    } 
    else {
        mantissa2 |= (1 << 10);
        exp2 = exp2 - 127; //remove bias
    }


    int s = sign1 ^ sign2;
    int e = exp1 + exp2 + 127;
    long long mprod = (long long) mantissa1 * (long long) mantissa2;
    int m, shift;

    int guard, sticky;

    // normalization step
    if (mprod & (1LL << 21)) {
        m = mprod >> 11;
        guard  = (mprod >> 10) & 1;
        sticky = (mprod & ((1 << 10) - 1)) != 0;
        e++;
    }
    else {
        m = mprod >> 10;
        guard  = (mprod >> 9) & 1;
        sticky = (mprod & ((1 << 9) - 1)) != 0;
    }


    // if overflows to infinity
    if (e >= 255) {  
        result = (s << 18) | (0xFF << 10); 
        
        return result;
    }

    

    // mantissa will never underflow

    // in case value becomes subnormal
    if (e <= 0) {
        int shift = 1 - e;
        if (shift < 32) {
            // shift mantissa into subnormal range
            m >>= shift;
        } else {
            m = 0;
        }
        e = 0;
    }

    //printf("%d\n", e);

    if (guard && (sticky || (m & 1))) {
        m++;
        if (m == (1 << 11)) {
            m >>= 1;
            e++;
        }
    }

    int mant_out = m & 0x3FF; // keep only 10 LSBs (remove implied 1)

    // reconstruct number and return
    result = (s << 18) | (e << 10) | (mant_out);

    
    return result;
}

tf32 tf32_div(tf32 a, tf32 b) {
    tf32 result = 0;
    
    // Extract sign bits
    int sign1 = (a >> 18) & 1;
    int sign2 = (b >> 18) & 1;

    // extract exps
    int exp1 = 0;
    int exp2 = 0;

    exp1 = (a >> 10) & 0xFF;
    exp2 = (b >> 10) & 0xFF;


    // extract mantissas
    int mantissa1 = 0;
    int mantissa2 = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa1 |= ((a >> i) & 1) << i; // adjust
        mantissa2 |= ((b >> i) & 1) << i;
    }


    // NaN and infinity check
    // if one of them is NaN, return NaN
    if (exp1 == 255 && mantissa1 != 0)
    {
        result = (sign1 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // when number 2 is NaN
    if (exp2 == 255 && mantissa2 != 0){
        result = (sign2 << 18) | (0xFF << 10) | 1;

        return result;
    }

    // infinity check (both)
    // if dividing infinity by infinity, return NaN
    if (((exp1 == 255 && mantissa1 == 0) && (exp2 == 255 && mantissa2 == 0)))
    {
        // +/- NaN
        result = ((sign1 ^ sign2) << 18) | (0xFF << 10) | 1;

        return result;
    }

    // if dividing infinity by different numbers
    if (exp1 == 255 && mantissa1 == 0){
        // return infinity if dividing infinity by any zero
        if (exp2 == 0 && mantissa2 == 0){
            result = (sign1 << 18) | (0xFF << 10);

            return result;
        }
        else {  // any other number that is nonzero, noninfty, nonNaN -> keep sign
            result = ((sign1 ^ sign2) << 18) | (0xFF << 10);

            return result;
        }
    }

    // Zero check
    if (exp1 == 0 && mantissa1 == 0) {
        // dividing zero by zero = positive NaN
        if (exp2 == 0 && mantissa2 == 0) {
            result = (0 << 18) | (0xFF << 10) | 1;

            return result;
        }
        else {  // infinity or normal value
            // return +/- zero
            result |= ((sign1 ^ sign2) << 18);

            return result;
        }
    }

    // normal values divided by zero or infinity
    // divided by zero => +/- infinity
    if (exp2 == 0 && mantissa2 == 0) {
        result = ((sign1 ^ sign2) << 18) | (0xFF << 10);

        return result;
    }

    // divided by infinity => +/- zero
    if (exp2 == 255 && mantissa2 == 0){
        result = ((sign1 ^ sign2) << 18);

        return result;
    }

    // end of NaN and infinity check

    // get implicit leading 1 except for small num
    if (exp1 != 0) mantissa1 |= (1 << 10);
    if (exp2 != 0) mantissa2 |= (1 << 10);

    // compute exp value
    int e = exp1 - exp2 + 127;
    int s = sign1 ^ sign2;

    // Newton Raphson Algorithm
    // Scale mantissa into fixed point value
    int x = mantissa2 << (20 - 10);

    // make reciprocal of b in fixed point
    int fp_shift = 20;
    
    // convert mantissa to fixed point
    long long m2_fixed = ((long long)mantissa2 << fp_shift) / (1 << 10);
    int y = ( (3 << (fp_shift-1)) - (m2_fixed >> 1) ); // fp starting point (1 didn't work :/)

    // algorithm
    for (int i = 0; i < 5; i++) {
        long long prod = ((long long) m2_fixed * y) >> fp_shift;
        long long term = ((1LL << (fp_shift + 1)) - prod); // (2 - m2*y)
        y = (int)(((long long) y * term) >> fp_shift);

    }

    // multiply mantissa and y
    long long m1_fixed = ((long long)mantissa1 << fp_shift) / 1024;
    long long M = (m1_fixed * (long long)y) >> fp_shift;


    // Normalize values
    if (M >= (2LL << fp_shift)) { M >>= 1; e++; }
    else if (M < (1LL << fp_shift)) { M <<= 1; e--; }
    
    // fractional part to round
    long long frac = M - (1LL << fp_shift);


    // Round mantissa
    int res   = (int)((frac >> (fp_shift - 10)) & 0x3FF);   // bits to keep
    int guard = res & 1;    // LSB of result
    int roundb = (int)((frac >> (fp_shift - 11)) & 1);  // first removed
    int sticky = ( (frac & ((1LL << (fp_shift - 11)) - 1)) != 0 );  // OR of rest

    // round to even
    if (roundb && (sticky || guard)) {
        res++;
        if (res == (1 << 10)) {          // carry into hidden 1
            res = 0;
            e++;
        }
    }

    // reconstruct value
    result = (s << 18) | ((e & 0xFF) << 10) | (res & ((1 << 10) - 1));

    return result;
}

int tf32_compare(tf32 a, tf32 b) {
    int result = 0;
    // 1 if a > b, 0 if a == b, -1 if a < b
    // Extract sign bits
    int sign1 = (a >> 18) & 1;
    int sign2 = (b >> 18) & 1;

    // extract exps
    int exp1 = 0;
    int exp2 = 0;

    exp1 = (a >> 10) & 0xFF;
    exp2 = (b >> 10) & 0xFF;


    // extract mantissas
    int mantissa1 = 0;
    int mantissa2 = 0;

    for (int i = 9; i >= 0; i--)
    {
        mantissa1 |= ((a >> i) & 1) << i; // adjust
        mantissa2 |= ((b >> i) & 1) << i;
    }

    // NaN, 0, infinity check
    // if there exists any NaN value
    if ((exp1 == 255 && mantissa1 != 0) || (exp2 == 255 && mantissa2 != 0)) {
        return -2;
    }

    
    if ((exp1 == 255 && mantissa1 == 0)) {
        // if a is positive infinity
        if (sign1 == 0){
            // if b is positive infinity
            if (exp2 == 255 && mantissa2 == 0 && sign2 == 0) {
                return 0;
            }
            else {  // if not, guaranteed to be a > b
                return 1;
            }
        }

        // if a is negative infinity
        if (sign1 == 1){
            // if b is negative infinity
            if (exp2 == 255 && mantissa2 == 0 && sign2 == 1) {
                return 0;
            }
            else {  // if not, guaranteed to be a < b
                return -1;
            }
        }
    }

    // if both are zeroes return 0
    if ((exp1 == 0 && mantissa1 == 0) && (exp2 == 0 && mantissa2 == 0)) {
        return 0;
    }

    // end of NaN, 0, infinity check

    // checking normal values...
    // normalize
    if (exp1 != 0) mantissa1 |= (1 << 10);
    if (exp2 != 0) mantissa2 |= (1 << 10);
    
    // check signs first
    if (sign1 == 0 && sign2 == 1) {
        return 1;
    }

    if (sign1 == 1 && sign2 == 0) {
        return -1;
    }

    // compare exp next
    if (exp1 > exp2) return (sign1 == 0) ? 1 : -1;  // if positive, higher exp is greater
    if (exp1 < exp2) return (sign1 == 0) ? -1 : 1;

    // compare mantissas next
    if (mantissa1 > mantissa2) return (sign1 == 0) ? 1 : -1;
    if (mantissa1 < mantissa2) return (sign1 == 0) ? -1: 1;

    // else return 0 (equal)
    return 0;
}
