#include "wjd_math.h"

int nextpwr2(int n)
{
  if(ISPWR2(n))
    return n;
  // n is assumed to be a 32-bit integer, so 5 shifts are sufficient
  n = (n >> 1) | n;
  n = (n >> 2) | n;
  n = (n >> 4) | n;
  n = (n >> 8) | n;
  n = (n >> 16) | n;
  // we should now have the a block of binary '1's the size of the original n
  return (n+1);
}

int isqrt(double x)
{
  if(x < 1)
    return 0;

  int i = 0;
  while(i*i <= x) i++;
  return (i-1);
}
