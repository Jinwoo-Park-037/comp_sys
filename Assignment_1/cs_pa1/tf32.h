
#ifndef TF32_H
#define TF32_H

typedef unsigned int tf32;

tf32   int2tf32(int in);
int    tf322int(tf32 in);
tf32   double2tf32(double in);
double tf322double(tf32 in);

tf32   tf32_add(tf32 a, tf32 b);
tf32   tf32_mul(tf32 a, tf32 b);
tf32   tf32_div(tf32 a, tf32 b);

int    tf32_compare(tf32 a, tf32 b);

// Helper function to output binary pattern of tf32 to stdout
// Be careful! This function doesn't put '\n' to stdout. 
extern void put_tf32(tf32 in);

#endif
