#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long
void main()
{
	 long a, b, c, d;
	 a = 4;
	 c = 5;
	 d = 6;
	 b = 38 + a;
	 b = c * d - c / d;
	 if (a < 0) {
	 	b = -1;
	 	a = -a;
	 } else {
	 	b = 1;
	 	c = c * d - c / d;
	 }
	 WriteLong(c);
	 while (a > 0) {
	 	b = b * 2;
	 	a = a / 2;
	 }
	 WriteLong(b);
	 c = c * d - c / d;
	 WriteLong(c);
	 WriteLine();
} 