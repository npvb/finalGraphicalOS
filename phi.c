#include "lib.h"

main()
{
	int i,j,k=1,l;

	setup();
	for (i=0; i<100; i++)
	{
		printstring("Hi\r\n\0");
		for (j=0; j<10000; j++)
		{
			for (l=0; l<10; l++)
				k=k*2;
		}
	}
	exit();
}
