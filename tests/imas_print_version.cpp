#include <stdio.h>
#include <string.h>
#include <al_lowlevel.h>

int main(int argc, char *argv[])
{
	int iRet = 0;

	printf("AL version:\t%s\n", getALVersion());
	printf("DD version:\t%s\n", getDDVersion());
	printf("\n");

	return iRet;
}
