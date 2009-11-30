#include <stdio.h>

int ret_test()
{
	char buf[10];

	return gets(buf);
}

int main(int argc, char **argv)
{
	return ret_test();
}
