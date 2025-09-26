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
    int sign = (in >> 31) & 1;

    // extract exp
    int exp = 0;

    for (int i = 7; i >= 0; i++)
    {
        exp |= ((in >> (i + 23)) & 1) << i;
    }


    // extract mantissa
    int mantissa = 0; 
    mantissa |= 1 << 23;   // starts with 1

    for (int i = 22; i >= 0; i--)
    {
        mantissa |= ((in >> i) & 1) << i;
    }
    

    
    // 1. Positive or negative infinity
    if (exp == 255 && mantissa == 1 << 23)
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
    if (sign == 1 && ((exp - 127) > 31 || ((exp -127) > 30 && mantissa > (1 << 23))))
    {
        int tmin = (1 << 31);
        return tmin;
    }


    // 4. Positive or Negative NaN



    // 5. Actual integer



    // Convert mantissa to int number
    

    

    return result;
}

tf32 double2tf32(double in) {
    tf32 result = 0;
    /*
    fill this function
    */
    return result;
}

double tf322double(tf32 in) {
    double result = 0.0;
    /*
    fill this function
    */
    return result;
}

tf32 tf32_add(tf32 a, tf32 b) {
    tf32 result = 0;
    /*
    fill this function
    */
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
