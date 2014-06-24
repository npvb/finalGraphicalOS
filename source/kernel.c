/*
kernel.c
Michael Black, 2007

This file contains all the C code comprising the kernel.
The kernel does the following:
-provides system calls for screen, keyboard, and file system (int 0x21)
-uses timer interrupt to provide multitasking
-supports multiple processes
-loads the shell

The kernel lives in memory between 10000 and 1ffff (hex)

This file must be compiled with bcc so that it runs in 16 bit real 
mode.  The -ansi flag must be set.
*/

/*we're limited to 8 processes if we stick to real mode and procs are 
64k*/
#define MAXPROCESSES 8

void scpy(char*,char*,int);
void dokernel();
void handleinterrupt21(char);
void handletimerinterrupt(short,short);
void initialize_process_table();

/*The kernel starts here*/
main()
{
	dokernel();
	terminate();
}

void dokernel()
{
	/*placeholder for the shell*/
	char file[12800];

	/*some friendly startup messages*/
	bios_printstr("\r\nBuild an Operating System from Scratch Project\r\n\0");
	bios_printstr("Written by Michael Black, December, 2007\r\n\n\0");
	bios_printstr("Starting the kernel\r\n\0");

	/*make the process table*/ 
	initialize_process_table();
	bios_printstr("Process table is initialized\r\n\0");

	/*install the ISR for interrupt 21*/
	makeinterrupt21();

	int21(1,"Interrupt 21 is installed\r\n\0");

	int21(1,"Kernel set up is completed\r\n\0");
	int21(1,"Loading the shell into memory\r\n\0");

	/*start the shell*/
	/*load it into file[]*/
	int21(3,"GSHELL\0",file);

	/*load it to memory and schedule it to be executed*/
	int21(6,file,12800,"GSHELL\0");

	/*install an ISR for the timer - this will call the scheduler*/
	maketimerinterrupt();

	int21(1,"Timer interrupt is installed\r\n\0");

	int21(1,"\r\nThe shell is starting.  The kernel will stay in the background.\r\n\n\0");
}

/*scpy is a utility function.  It copies a string src to string dst*/
void scpy(char* src, char* dst, int length)
{
	int i;
	for (i=0; i<length; i++)
		dst[i]=src[i];
}

/*definition of a process entry*/
struct process_table_entry
{
	/*active:
		0=no process here
		1=regular active process
		2=process is waiting for another process to end
		3=kernel process, don't run it*/
	char active;
	/*if active==2, waiton tells who the process is waiting on*/
	char waiton;
	/*base address of the process: 
		it lives from segment to segment+0xffff*/
	unsigned short segment;
	/*process's stack pointer value*/
	unsigned short sp;
};

/*the process table*/
struct process_table_entry process_table[MAXPROCESSES];
/*the current process is 0 - the kernel*/
int current_process=0;

/*this initializes the process table and sets up the kernel proc*/
void initialize_process_table()
{
	int i;

	for (i=0; i<MAXPROCESSES; i++)
	{
		/*all procs are inactive*/
		process_table[i].active=0;
		/*procs start at memory (id+1) * 0x10000*/
		process_table[i].segment=(i+1)*0x1000;
		/*initial stack pointer is ff00*/
		process_table[i].sp=0xff00;
	}
	/*process 0 is the kernel*/
	process_table[0].active=3;
}

/*
interrupt 21 handler
type:
1: print string.  address1 = address of string to print
2: read string.  address1 = destination address of string to read
3: read file.  address1 = name, address2 = destination buffer
4: write file.  address1 = name, address2 = source buffer, address3 = number of sectors in file
5: delete file.  address1 = name
6: execute program in background.  address1 = program buffer, address2 = length of program (bytes)
7: terminate program.
8: execute program in foreground.  address1 = program buffer, address2 = length of program (bytes)
9: kill program.  address1 = process id number
*/

/*read string reads characters one at a time and puts them in a buffer*/
/*when return is pressed, it returns with the finished buffer*/
void readstring(char* buffer)
{
	int index=0;
	char c=0x00;

	/*wait for ENTER (0xd)*/
	while (c!=0xd)
	{
		/*read a character from the keyboard*/
		c=readchar();
		/*if not backspace, put it in the buffer & print it*/
		if (c!=8)
		{
			printchar(c);
			buffer[index]=c;
			index++;
		}
		/*if it is backspace, space over the last character*/
		else if (index>0)
		{
			printchar(c);
			printchar(0x20);
			printchar(c);
			index--;
		}
	}
	/*put in a line feed and print it*/
	buffer[index]=0xa;
	printchar(buffer[index]);
	/*terminate with a 0*/
	buffer[index+1]=0;
}

/*given a name of a file and a buffer holding the directory,
  find the file in the directory and return its starting index.*/
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

		/*each entry is 32 bytes*/
		index=index+0x20;
	}

	/*did we not find it? return -1*/
	if (i==16)
	{
		return -1;
	}

	return index;
}

/*computes a mod b*/
int mod(int a, int b)
{
	while(a>=b)
		a=a-b;
	return a;
}

/*computes a / b (integer result only) */
int div(int a, int b)
{
	int q=0;
	while (q*b<=a)
		q++;
	return q-1;
}

/*deletes the first file with the name stored in name[]*/
/*deleting involves two steps:
  1) turning all the file's map entries into 0 (sector is available)
  2) setting the first byte of the file name to 0 (dir entry is empty)
  note that it is unnecessary to actually wipe out the file*/
void delfile(char* name)
{
	int index;
	char dirsector[512];
	char mapsector[512];

	/*read the disk map and directory*/
	readsector(dirsector,3,0,0);
	readsector(mapsector,2,0,0);
	/*find the file in the directory*/
	index=findname(name,dirsector);
	if (index==-1)
		return;

	/*set the file name to deleted*/
	dirsector[index]=0x00;
	index=index+6;
	/*set all the file's map entries to 0*/
	while(dirsector[index]!=0x00)
		mapsector[dirsector[index++]]=0x00;
	
	/*write back the map and directory*/
	writesector(dirsector,3,0,0);
	writesector(mapsector,2,0,0);
}

/*read the file with name name[] into buffer[]*/
void readfile(char* name, char* buffer)
{
	char sector[512];
	int i,j,index,bufferindex=0;
	int sec,head,cyl;

	/*load the directory into sector*/
	readsector(sector,3,0,0);

	/*get the index of the file name*/
	index=findname(name,sector)+6;

	/*-1+6 is 5 - this means the file doesn't exist*/
	if (index==5)
		return;

	/*read each sector into buffer*/
	while(sector[index]!=0x00)
	{
		/*convert to CHS*/
		sec=mod(sector[index],0x12)+1;
		head=mod(div(sector[index],0x12),2);
		cyl=div(sector[index],0x24);
		readsector(buffer+bufferindex,sec,head,cyl);

		index++;
		/*step forward by 512 (1 sector)*/
		bufferindex=bufferindex+512;
	}

}

/*write file writes the bytes in buffer[] to a new file name[]*/
/*sectorlength tells how big this file must be in sectors*/
void writefile(char* name, char* buffer, int sectorlength)
{
	char dirsector[512];
	char mapsector[512];
	int i,j,k,index,sec,cyl,head;

	/*file sizes are limited to 25 sectors*/
	if (sectorlength>25)
		return;

	/*get directory and map*/
	readsector(dirsector,3,0,0);
	readsector(mapsector,2,0,0);

	/*find first available entry*/
	for (i=0; i<512; i=i+0x20)
	{
		if (dirsector[i]==0x00)
			break;
	}
	/*is directory full?*/
	if (i==512)
		return;

	/*copy name to directory, filling remainder with spaces*/
	for (j=0; j<6; j++)
		dirsector[i+j]=0x20;
	for (j=0; j<6; j++)
	{
		if (name[j]==0x00)
			break;
		dirsector[i+j]=name[j];
	}

	i=i+6;
	index=0;

	/*write the file*/
	for (j=0; j<sectorlength; j++)
	{
		/*find free sectors for file*/
		for (k=0; k<256; k++)
		{
			if (mapsector[k]==0)
				break;
		}
		if (k==256)
			break;
		/*set the map entry for the sector to "full"*/
		mapsector[k]=0x46;
		/*add that sector number to the file's dir entry*/
		dirsector[i]=(char)k;
		i++;
		
		/*compute CHS*/
		sec=mod(k,0x12)+1;
		head=mod(div(k,0x12),2);
		cyl=div(k,0x24);
		/*write a sector's worth to the disk*/
		writesector(buffer+index,sec,head,cyl);

		index=index+512;
		
	}
	/*set the next sector in the dir entry to 0.*/
	dirsector[i]=0;
	/*write back the map and directory*/
	writesector(mapsector,2,0,0);
	writesector(dirsector,3,0,0);
}

/*this var used in the next two functions has to be in data, not stack*/
int procid;

/*execute program finds an available process table entry and puts the program in the appropriate segment*/
void executeprogram(char* filebuffer, int length, int foreground)
{
	char* programbuffer;
	int i,j;
	short seg;

	/*set the current data segment to the kernel so we can read the 
		process list*/
	setdatasegkernel();

	/*find an empty space for a process*/
	for (i=0; i<MAXPROCESSES; i++)
		if (process_table[i].active==0)
			break;
	/*if the memory is full, we can't execute*/
	if (i==MAXPROCESSES)
	{
		restoredataseg();
		return;
	}
	/*set the process entry to active, set the stack pointer*/
	seg=process_table[i].segment;
	process_table[i].active=1;
	process_table[i].sp=0xff00;

	/*get the id of the process that called execute*/
	procid=getprocessid();
	/*if the new process is to occupy the foreground, make the 
		caller sleep*/
	if (foreground==1)
	{
		process_table[procid].active=2;
		process_table[procid].waiton=i;
	}

	/*set the data segment back to the caller's segment*/
	restoredataseg();

	/*load the new process into memory*/
	loadprogram(seg,filebuffer,length);

}

/*Deactivate the process and wait for a process change*/
void terminateprogram()
{
	int i;

	/*set the data segment to the kernel so we can read the process 
		table*/
	setdatasegkernel();
	/*get the id of the caller*/
	procid=getprocessid();
	/*set its entry in the table to empty*/
	process_table[procid].active=0;

	/*check if anyone is waiting on the caller. if so, wake them up*/
	for (i=0; i<MAXPROCESSES; i++)
		if (process_table[i].active==2 && process_table[i].waiton==procid)
			process_table[i].active=1;

	/*set the data segment back to the caller*/
	restoredataseg();
	/*just busy wait until the timer moves us along*/
	terminate();
}

/*terminates the process proc*/
void kill(int proc)
{
	int i;

	/*if proc isn't a valid process, ignore it*/
	if (proc<0 || proc>=MAXPROCESSES)
		return;

	/*set the data segment to the kernel to read the table*/
	setdatasegkernel();
	/*set proc's entry to empty*/
	process_table[proc].active=0;

	/*if anybody is waiting on proc, wake them up*/
	for (i=0; i<MAXPROCESSES; i++)
		if (process_table[i].active==2 && process_table[i].waiton==proc)
			process_table[i].active=1;

	/*restore the data segment to the caller's*/
	restoredataseg();
}

/*when an interrupt 21 happens, this function is called*/
/*type tells what to do.  address1,2,3 are parameters - the values 
  passed in bx,cx, and dx */
void handleinterrupt21(char type, char* address1, char* address2, char* address3)
{
	if (type==1)
		bios_printstr(address1);
	else if (type==2)
		readstring(address1);
	else if (type==3)
		readfile(address1,address2);
	else if (type==4)
		writefile(address1,address2,address3);
	else if (type==5)
		delfile(address1);
	else if (type==6)
		executeprogram(address1,address2,0);
	else if (type==7)
		terminateprogram();
	else if (type==8)
		executeprogram(address1,address2,1);
	else if (type==9)
		kill(address1);
	else
		bios_printstr("ERROR: Invalid interrupt 21 code\r\n\0");
}


/*perform a process switch*/
void handletimerinterrupt(short segment, short sp)
{
	int i,cntr=79;
	/*save current process*/
	process_table[current_process].segment=segment;
	process_table[current_process].sp=sp;

	/*print out the active processes at the top of the screen*/
	for (i=MAXPROCESSES-1; i>=0; i--)
	{
		if (process_table[i].active>0)
		{
			printtop('P',cntr-5);
			printtop((char)(i+0x30),cntr-4);
			printtop(' ',cntr-3);
			if (process_table[i].active==1)
				printtop('A',cntr-2);
			else if (process_table[i].active==2)
				printtop('W',cntr-2);
			else if (process_table[i].active==3)
				printtop('K',cntr-2);
			printtop(' ',cntr-1);
			printtop(' ',cntr);
			cntr=cntr-6;
		}
	}

	printtop(' ',cntr);
	printtop(' ',cntr-1);
	printtop(':',cntr-2);
	printtop('s',cntr-3);
	printtop('e',cntr-4);
	printtop('s',cntr-5);
	printtop('s',cntr-6);
	printtop('e',cntr-7);
	printtop('c',cntr-8);
	printtop('o',cntr-9);
	printtop('r',cntr-10);
	printtop('P',cntr-11);

	/*find an active process round robin style*/
	i=current_process;
	do
	{
		i++;
		if (i==MAXPROCESSES)
			i=0;
	} while(process_table[i].active!=1);

	current_process=i;

	/*restore that process*/
	timer_restore(process_table[current_process].segment,process_table[current_process].sp);
	
}
