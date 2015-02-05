
#define size 10
#define BQINSERT 0
#define BQREMOVE BQINSERT+ 1
#define BQEND BQREMOVE+ 1
#define BQDATA BQEND+ 1

unsigned char array[size];

unsigned char sizebq1(unsigned char *q);
unsigned char sizebq1(unsigned char *q)
{
	return (unsigned char)((unsigned char)(q[BQEND]) - (unsigned char)BQDATA);
}

unsigned char sizebq2(unsigned char *q);
unsigned char sizebq2(unsigned char *q)
{
	unsigned char b = q[BQEND];
	
	b -= BQDATA;
	return b;
}

int test(void);
int test(void)
{
	return (sizebq1(array) == sizebq2(array));
}
