/*shell.c
Michael Black, 2007

Creates the shell and handles the shell commands.
The shell is automatically loaded by the kernel
*/

#include "lib.h"

void doshell();

/*the shell starts here*/
main()
{
	/*enable interrupts*/
	setup();
	/*start the main shell routine*/
	doshell();
	/*terminate - this shouldn't run*/
	exit();
}

/*compares command with line.  if they are equal, returns 1, else 0*/
int iscommand(char* line, char* command)
{
	int i=0;

	/*step through until reach end of string*/
	while(command[i]!='\0')
	{
		if (line[i]!=command[i])
			return 0;
		i++;
	}
	return 1;
}

/*returns the first argument after the command*/
char* getargument(char* line)
{
	int i=0;
	int j=0;

	/*find the space*/
	while(line[i]!=' ')
		i++;
	i++;

	/*find the end and replace it with a \0*/
	j=i;
	while(line[j]!=0xd)
	{
		j++;
		if (j-i==6)
			break;
	}
	line[j]=0x0;

	return line+i;
}

/*execute a program.  fore is 1 if the shell is meant to be paused while the program runs*/
void doexecute(char* line, int fore)
{
	char program[12800];

	/*extract the name from the command line*/
	char* name=getargument(line);
	/*check that a program is loaded by putting 0 in the first byte.  if it stays, wasn't loaded*/
	/*I guess it's bad luck if the program starts with 0*/
	program[0]=0x0;
	readfile(name,program);
	if (program[0]==0x0)
	{
		printstring("Program not found\r\n\0");
	}
	else
	{
		/*use fore to pick one of the library functions*/
		if (fore==1)
			executeprogram(program,12800);
		else
			executeprogrambackground(program,12800);
	}
}

/*load the file and print out the contents.  assume that's it's ASCII*/
void dotype(char* line)
{
	char file[12800];

	char* name=getargument(line);
	file[0]=0x1;
	readfile(name,file);
	if (file[0]==0x1)
	{
		printstring("File not found\r\n\0");
	}
	else
	{
		printstring(file);
	}
}

/*print out a help screen*/
void dohelp()
{
	printstring("The commands are: \r\n\0");
	printstring("     CLS:  Clear the screen\r\n\0");
	printstring("     COPY: Copy one file to another with a different name\r\n\0");
	printstring("     CREATE <FILE_NAME>:  Create a text file FILE_NAME\r\n\0");
	printstring("     DELETE <FILE_NAME>:  Delete the file FILE_NAME\r\n\0");
	printstring("     DIR:  Print out the directory\r\n\0");
	printstring("     EXEC <PROGRAM_NAME>:  Run the program PROGRAM_NAME\r\n\0");
	printstring("     EXECBACK <PROGRAM_NAME>:  Run PROGRAM_NAME as a background process\r\n\0");
	printstring("     HELP or ?:  Print this message\r\n\0");
	printstring("     KILL <PROCESS_NUMBER>:  Terminate the process with the id PROCESS_NUMBER\r\n\0");
	printstring("     TYPE <FILE_NAME>:  Print out the contents of file FILE_NAME\r\n\0");
}

/*allow the user to create a text file*/
void docreate(char* line)
{
	char file[12800];
	int index=0;
	char* name=getargument(line);
	char c=0;

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
}

/*delete a file*/
void dodelete(char* line)
{
	char* name=getargument(line);
	/*make the system call*/
	deletefile(name);
}

/*print out the directory*/
void dodir()
{
	char dirsector[512];
	char mapsector[512];
	char name[7];
	int i,j,index,files;

	/*read the directory sector*/
	readsector(2,dirsector);

	printstring("Directory:\r\n\0");
	index=0;
	files=0;
	/*the file system allows no more than 16 files*/
	for (i=0; i<16; i++)
	{
		/*check if the file exists*/
		if (dirsector[index]!=0)
		{
			/*print hte name*/
			for (j=0; j<6; j++)
				name[j]=dirsector[index+j];
			name[6]=0;
			printstring(name);

			/*calculate the file size in sectors from the directory entry*/
			j=0;
			while(dirsector[index+j+6]!=0)
				j++;

			printstring("  \0");
			printnumber(j);
			printstring(" sectors\r\n\0");
			files++;
		}
		/*move to the next entry*/
		index=index+0x20;
	}
	printnumber(files);
	printstring(" files total (out of 16 maximum)\r\n\0");

	/*read the map sector to find out how many free sectors are left*/
	readsector(1,mapsector);
	j=0;
	for (i=0; i<256; i++)
	{
		if (mapsector[i]==0x0)
			j++;
	}

	printnumber(j);
	printstring(" sectors available\r\n\0");
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

	/*prompt for the first file name*/
	printstring("Name of the source file? \0");
	readstring(line);
	for (i=0; i<6; i++)
	{
		sname[i]=line[i];
		if (line[i]==0xd)
			break;
	}
	sname[i]=0x0;

	/*make sure it exists - find the directory entry*/
	readsector(2,dirsector);
	index=findname(sname,dirsector);
	if (index==-1)
	{
		printstring("File not found\r\n\0");
		return;
	}

	/*read the source file*/
	readfile(sname,file);

	/*prompt for the destination file name*/
	printstring("Name of the destination file? \0");
	readstring(line);
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
}

/*kill a process*/
void dokill(char* line)
{
	/*make the system call*/
	char* arg=getargument(line);
	int21(9,arg[0]-0x30);
}

/*clear the screen*/
void doclear()
{
	clearscreen();
	setcursor(0,0);
}

/*this is the main routine*/
void doshell()
{
	char line[80];

	/*run forever - the shell shouldn't end*/
	while(1==1)
	{
		/*read in a line*/
		printstring("SHELL>\0");
		readstring(line);

		/*match it against each possible command*/
		/*if the user presses return, ignore it*/
		if (line[0]==0xd)
			continue;
		else if (iscommand(line,"CLS\0")==1)
			doclear();
		else if (iscommand(line,"cls\0")==1)
			doclear();
		else if (iscommand(line,"COPY\0")==1)
			docopy();
		else if (iscommand(line,"copy\0")==1)
			docopy();
		else if (iscommand(line,"CREATE \0")==1)
			docreate(line);
		else if (iscommand(line,"create \0")==1)
			docreate(line);
		else if (iscommand(line,"DELETE \0")==1)
			dodelete(line);
		else if (iscommand(line,"delete \0")==1)
			dodelete(line);
		else if (iscommand(line,"DIR\0")==1)
			dodir();
		else if (iscommand(line,"dir\0")==1)
			dodir();
		else if (iscommand(line,"EXEC \0")==1)
			doexecute(line,1);
		else if (iscommand(line,"exec \0")==1)
			doexecute(line,1);
		else if (iscommand(line,"EXECBACK \0")==1)
			doexecute(line,0);
		else if (iscommand(line,"execback \0")==1)
			doexecute(line,0);
		else if (iscommand(line,"HELP\0")==1)
			dohelp();
		else if (iscommand(line,"help\0")==1)
			dohelp();
		else if (line[0]=='?')
			dohelp();
		else if (iscommand(line,"TYPE \0")==1)
			dotype(line);
		else if (iscommand(line,"type \0")==1)
			dotype(line);
		else if (iscommand(line,"KILL \0")==1)
			dokill(line);
		else if (iscommand(line,"kill \0")==1)
			dokill(line);
		else
			printstring("Command not found\r\n\0");
		printstring("\r\n\0");
	}
}

