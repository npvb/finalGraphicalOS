#include "lib.h"

void initialize_screen();

main()
{
	setvideo(1);

	initialize_screen();

	while(getchar()!=0xd);

	setvideo(0);

	exit();
}

void initialize_screen()
{
	int r,c;

	setvideographics();
	for (r=0; r<200; r++)
		for (c=0; c<320; c++)
			drawpixel(0x32,320*r+c);
}
