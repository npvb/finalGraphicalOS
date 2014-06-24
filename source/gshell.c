/*gshell.c
Michael Black, 2007

Creates a text-user-interface shell and handles the shell commands.
The shell is automatically loaded by the kernel
*/

#include "lib.h"
#define COMMANDS 8
#define TC 3*16+1	/*text color*/
#define HC 4*16+12	/*highlighted color*/
#define BC 1*16		/*boundary color*/

void doshell();
void drawInterface();
void dofileselect(char*);
void clearrightpanel();

/*the shell starts here*/
main()
{
	/*enable interrupts*/
	setup();

	/*wait for carriage return*/
	printstring("\r\nPress ENTER to continue");
	while(getchar()!=0xd);

	/*draw the interface*/
	drawInterface();

	/*start the main shell routine*/
	doshell();

	/*terminate - this shouldn't run*/
	exit();
}

/*exit the shell to a command line*/
void doexit()
{
	char program[12800];

	clearscreen();
	setcursor(0,0);

	readfile("SHELL\0",program);
	executeprogrambackground(program,12800);
	exit();
}

/*execute a program in the foreground*/
void doexecute()
{
	char program[12800];
	char name[7];

	dofileselect(name);
	readfile(name,program);

	clearscreen();
	setcursor(0,0);
	executeprogram(program,12800);

	/*wait for carriage return*/
	while(getchar()!=0xd);

	/*draw the interface*/
	drawInterface();

	setstring("Press 1 to move up, 2 to move down, ENTER to select",TC,24,0);
}

/*load the file and print out the contents.  assume that's it's ASCII*/
void dotype()
{
	char file[12800];
	char name[7];

	dofileselect(name);
	readfile(name,file);

	clearscreen();
	setcursor(0,0);
	printstring(file);

	printstring("\r\n\nPress ENTER to continue");

	/*wait for carriage return*/
	while(getchar()!=0xd);

	/*draw the interface*/
	drawInterface();

	setstring("Press 1 to move up, 2 to move down, ENTER to select",TC,24,0);
}

/*allow the user to create a text file*/
void docreate()
{
        char file[12800];
	char line[80];
	int i;
        int index=0;
        char name[7];
        char c=0;

	/*prompt for the destination file name*/
	setstring("Name of the destination file?                                              ",TC,24,0);
	setcursor(24,30);
	readstring(line);

	for (i=0; i<6; i++)
	{
		name[i]=line[i];
		if (line[i]==0xd)
			break;
	}
	name[i]=0x0;

	clearscreen();
	setcursor(0,0);

        printstring("Enter your text.  End the file by typing CTRL-C.\r\n\n\0");

        /*terminates when user presses CTRL-C - 3) */
        while(c!=0x3)
        {
                /*get a character and store it*/
                c=getchar();
                file[index++]=c;
                /*echo it back if it isn't CTRL-C*/
                if (c!=0x3)
                        putchar(c);
                /*if the user presses ENTER, follow the CR with a LF*/
                if (c==0xd)
                {
                        file[index++]=0xa;
                        putchar(0xa);
                }
        }
        /*terminate the file with a 0*/
        file[index-1]=0x0;

        /*write it to the disk.  Round up the sector*/
        writefile(name,file,div(index,512)+1);
        printstring("\r\n\0");

	/*draw the interface*/
	drawInterface();

	setstring("Press 1 to move up, 2 to move down, ENTER to select",TC,24,0);
}

/*print out a help 
message*/
void dohelp(int choice)
{
	int c;
	for (c=0; c<80; c++)
		setchar(' ',TC,24,c);
	if (choice==0)
		setstring("Copy a file to another with a different name",TC,24,0);
	else if (choice==1)
		setstring("Create a new text file",TC,24,0);
	else if (choice==2)
		setstring("Delete a file",TC,24,0);
	else if (choice==3)
		setstring("List the names of the files on the disk",TC,24,0);
	else if (choice==4)
		setstring("Execute a program",TC,24,0);
	else if (choice==5)
		setstring("Leave the graphical shell and go to a command line shell",TC,24,0);
	else if (choice==6)
		setstring("Terminate a process",TC,24,0);
	else if (choice==7)
		setstring("Print out a text file",TC,24,0);
	else if (choice==-1)
		setstring("Press 1 to move up, 2 to move down, ENTER to select",TC,24,0);
}

/*delete a file*/
void dodelete()
{
	char name[7];

	dofileselect(name);
	deletefile(name);

	setstring("Done.                                                                      ",TC,24,0);
	clearrightpanel();
}

/*highlight a file name*/
void highlightSelect(int choice)
{
	int c;
	int r;
	for (r=5; r<5+16; r++)
	{
		for (c=45; c<=51; c++)
		{
			putInMemory(0xB800,(r*80+c)*2+1,TC);
		}
	}
	for (c=45; c<=51; c++)
		putInMemory(0xB800,((choice+5)*80+c)*2+1,HC);
}

/*select a file graphically*/
void dofileselect(char* filename)
{
	char name[16][7];
	char dirsector[512];
	int choice=0;
	int i,j,index,files,row=5;
	char key;

	/*read the directory sector*/
	readsector(2,dirsector);

	index=0;
	files=0;
	/*the file system allows no more than 16 files*/
	for (i=0; i<16; i++)
	{
		/*check if the file exists*/
		if (dirsector[index]!=0)
		{
			/*save the name*/
			for (j=0; j<6; j++)
				name[files][j]=dirsector[index+j];
			name[files][6]=0;
			setstring(name[files],TC,row,45);

			files++;
			row++;
		}
		/*move to the next entry*/
		index=index+0x20;
	}

	choice=0;
	highlightSelect(choice);

	while(1)
	{
		/*read in a direction: up, down, enter*/
		key=getchar();
		if (key!=0x31 && key!=0x32 && key!=0xd && key!='?')
			continue;

		if (key=='?')
		{
			dohelp(-1);
			continue;
		}

		if (key==0x31)
		{
			choice=choice-1;
			if (choice<0)
				choice=files-1;
			highlightSelect(choice);
			continue;
		}
		if (key==0x32)
		{
			choice=choice+1;
			if (choice==files)
				choice=0;
			highlightSelect(choice);
			continue;
		}
		
		for (i=0; i<7; i++)
			filename[i]=name[choice][i];
		return;
	}
}

/*print out the directory*/
void dodir()
{
	char dirsector[512];
	char mapsector[512];
	char name[7];
	char num[10];
	int i,j,index,files,row=6;

	/*read the directory sector*/
	readsector(2,dirsector);

	index=0;
	files=0;
	/*the file system allows no more than 16 files*/
	for (i=0; i<16; i++)
	{
		/*check if the file exists*/
		if (dirsector[index]!=0)
		{
			/*print the name*/
			for (j=0; j<6; j++)
				name[j]=dirsector[index+j];
			name[6]=0;
			setstring(name,TC,row,45);

			/*calculate the file size in sectors from the directory entry*/
			j=0;
			while(dirsector[index+j+6]!=0)
				j++;

			getnumberstring(num,j);
			setstring(num,TC,row,55);
			setstring("sectors",TC,row,58);
			files++;
			row++;
		}
		/*move to the next entry*/
		index=index+0x20;
	}

	/*read the map sector to find out how many free sectors are left*/
	readsector(1,mapsector);
	j=0;
	for (i=0; i<256; i++)
	{
		if (mapsector[i]==0x0)
			j++;
	}

	getnumberstring(num,j);
	setstring(num,TC,4,45);
	setstring("sectors available",TC,4,49);

	getnumberstring(num,files);
	setstring(num,TC,3,45);
	setstring("files",TC,3,48);
}

/*finds where a file name exists within the directory*/
int findname(char* name, char* sector)
{
        int i,j,index=0;

        /*search the directory until we find name*/
        /*after 16 entries, quit*/

        for (i=0; i<16; i++)
        {
                /*not a file?  continue*/
                if (sector[index]==0)
                {
                        index=index+0x20;
                        continue;
                }
                /*try to match name*/
                for (j=0; j<6; j++)
                {
                        if (name[j]=='\0' && sector[index+j]==0x20)
                        {
                                j=6;
                                break;
                        }
                        if (name[j]!=sector[index+j])
                                break;
                }

                /*name matches, break*/
                if (j==6)
                        break;

                index=index+0x20;
        }

        /*did we not find it?  then return -1*/
        if (i==16)
        {
                return -1;
        }

        return index;
}

/*copy a file*/
void docopy()
{
	char sname[7];
	char dname[7];
	char line[80];
	char file[12800];
	char dirsector[512];
	int index,i;

	dofileselect(sname);

	/*make sure it exists - find the directory entry*/
	readsector(2,dirsector);
	index=findname(sname,dirsector);

	/*read the source file*/
	readfile(sname,file);

	/*prompt for the destination file name*/
	setstring("Name of the destination file?                                              ",TC,24,0);
	setcursor(24,30);
	readstring(line);

	drawInterface();

	for (i=0; i<6; i++)
	{
		dname[i]=line[i];
		if (line[i]==0xd)
			break;
	}
	dname[i]=0x0;

	/*figure out how long the source file is*/
	i=0;
	index=index+6;
	while(dirsector[index]!=0x0)
	{
		index++;
		i++;
	}

	/*write the file*/
	writefile(dname,file,i);

	setstring("Done.                                                                      ",TC,24,0);

	clearrightpanel();
}

/*kill a process*/
void dokill()
{
	char p;

	setstring("Which process number?                                                 ",TC,24,0);
	p=getchar();

	/*make the system call*/
	int21(9,p-0x30);

	drawInterface();

	setstring("Done.                                                                      ",TC,24,0);
}

void drawInterface()
{
	int r,c;

	clearscreen();

	/*set screen to cyan*/
	for (c=0; c<80; c++)
		for (r=0; r<25; r++)
		{
			putInMemory(0xB800,(r*80+c)*2,' ');
			putInMemory(0xB800,(r*80+c)*2+1,TC);
		}
	setcursor(24,0);

	/*draw boundary*/
	for (c=0; c<80; c++)
	{
		setchar(' ',BC,1,c);	
		setchar(' ',BC,23,c);	
	}
	for (r=1; r<24; r++)
	{
		setchar(' ',BC,r,0);
		setchar(' ',BC,r,40);
		setchar(' ',BC,r,79);
	}

	/*draw title*/
	setstring("Graphical Shell (BOSS Project)",TC,0,0);

	/*list commands*/
	setstring("Copy",TC,5,10);
	setstring("Create",TC,7,10);
	setstring("Delete",TC,9,10);
	setstring("Directory",TC,11,10);
	setstring("Execute",TC,13,10);
	setstring("Exit to command line",TC,15,10);
	setstring("Kill",TC,17,10);
	setstring("Type",TC,19,10);

	setstring("Press 1 to move up, 2 to move down, ENTER to select",TC,24,0);
}

void clearrightpanel()
{
	int r,c;

	for (c=41; c<79; c++)
		for (r=2; r<23; r++)
			setchar(' ',TC,r,c);
}

void highlight(int choice)
{
	int c;
	int r;
	for (r=5; r<5+COMMANDS*2; r+=2)
	{
		for (c=10; c<=30; c++)
		{
			putInMemory(0xB800,(r*80+c)*2+1,TC);
		}
	}
	for (c=10; c<=30; c++)
		putInMemory(0xB800,((choice*2+5)*80+c)*2+1,HC);
}

/*this is the main routine*/
void doshell()
{
	int choice=0;
	int key=0;

	highlight(choice);

	/*run forever*/
	while(1)
	{
		/*read in a direction: up, down, enter*/
		key=getchar();
		if (key!=0x31 && key!=0x32 && key!=0xd && key!='?')
			continue;

		if (key=='?')
		{
			dohelp(-1);
			continue;
		}

		if (key==0x31)
		{
			choice=choice-1;
			if (choice<0)
				choice=COMMANDS-1;
			highlight(choice);
			dohelp(choice);
			continue;
		}
		if (key==0x32)
		{
			choice=choice+1;
			if (choice==COMMANDS)
				choice=0;
			highlight(choice);
			dohelp(choice);
			continue;
		}

		clearrightpanel();

		if (choice==0)
			docopy();
		else if (choice==1)
			docreate();
		else if (choice==2)
			dodelete();
		else if (choice==3)
			dodir();
		else if (choice==4)
			doexecute();
		else if (choice==5)
			doexit();
		else if (choice==6)
			dokill();
		else if (choice==7)
			dotype();
	
	}
}
