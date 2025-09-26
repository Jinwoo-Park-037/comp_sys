#include <stdio.h>
#include "tf32.h"

void put_tf32(tf32 in) {
    int i;
    for (i = 0; i < 19; ++i) {
        putchar((in & 0x40000U) ? '1' : '0');
        in <<= 1;
    }
}

int main(void) {
    int input1;
    tf32 input2;
    double input3;
    tf32 input4;
    tf32 input5, input6;
    tf32 input7, input8;
    tf32 input9, input10;
    tf32 input11, input12;
    union {double f; unsigned long long u;} result4;

    scanf("%d\n", &input1);
    printf("int2tf32(%d) = 0x%05x\n", input1, int2tf32(input1));
    scanf("%x\n", &input2);
    printf("tf322int(0x%05x) = %d\n", input2, tf322int(input2));
    scanf("%lf\n", &input3);
    printf("double2tf32(%lf) = 0x%05x\n", input3, double2tf32(input3));
    scanf("%x\n", &input4);
    result4.f = tf322double(input4);
    printf("tf322double(0x%05x) = %lf (0x%016llx)\n", input4, result4.f, result4.u);
    
    scanf("%x %x\n", &input5, &input6);
    printf("tf32_add(0x%05x, 0x%05x) = 0x%05x\n", input5, input6, tf32_add(input5, input6));
    scanf("%x %x\n", &input7, &input8);
    printf("tf32_mul(0x%05x, 0x%05x) = 0x%05x\n", input7, input8, tf32_mul(input7, input8));
    scanf("%x %x\n", &input9, &input10);
    printf("tf32_div(0x%05x, 0x%05x) = 0x%05x (%f)\n", input9, input10, tf32_div(input9, input10), tf322double(tf32_div(input9, input10)));

    scanf("%x %x\n", &input11, &input12);
    printf("tf32_compare(0x%05x, 0x%05x) = %d\n", input11, input12, tf32_compare(input11, input12));

    printf("put_tf32(0x%05x) is ", input1);
    put_tf32(input1);
    putchar('\n');

    return 0;
}

