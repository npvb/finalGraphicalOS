/*IMPLEMENTACIÓN DE FUNCIONES*/
#define NOTFOUND -1
#include "string.h"

void mkdir(char* path, char* name)
{

	int i;
		
	//load the disk map
	char map[512];
	//fseek(floppy,512,SEEK_SET);
	/*for(i=0; i<512; i++)
		map[i]=fgetc(floppy);*/

	//Cargamos el mapa
	readsector(map,2,0,0);

	//load the directory "/"
	char dir[512];
	/*fseek(floppy,512*2,SEEK_SET);
	for (i=0; i<512; i++)
		dir[i]=fgetc(floppy);*/
	readsector(dir,3,0,0);	
	char nombre[6];
	char found = 0;
	int x;
	/*Parsea el path*/
	for(int n=0;n<5;n++)
	{
		if(path[n] == '/')
			nombre[n] = 0x20;
		else
		{
			nombre[n] = path[n];
		}
	}

	for (x=0;x<15;x++)
	{
		for(int y=0;y<5;y++)
		{
			found = dir[x*16+y] == path[y];
			if(found == 0)
			{
				break;
			}
		}
		if(found == 1)
		{
			break;
		}
	}
	char sectorNumber = dir[x*16+6];
	readsector(dir,sectorNumber,0,0);



	//find a free entry in the directory
	for (i=0; i<512; i=i+0x20)
		if (dir[i]==0)
			break;
	if (i==512)
	{
		//printf("Not enough room in directory\n");
		return;
	}
	int dirindex=i;

	//copy the name over
	for (i=0; i<6; i++)
		dir[dirindex+i]=0x20;
	for (i=0; i<6; i++)
	{
		if(name[i]==0)
			break;
		dir[dirindex+i]=name[i];
	}

	dirindex=dirindex+6;

	//find free sectors and add them to the file
	int sectcount=0;

		if (sectcount==26)
		{
			//printf("Not enough space in directory entry for file\n");
			return;
		}

		//find a free map entry
		for (i=0; i<256; i++)
			if (map[i]==0)
				break;
		if (i==256)
		{
			//printf("Not enough room for file\n");
			return;
		}

		//mark the map entry as taken
		map[i]=0x44;

		//mark the sector in the directory entry
		dir[dirindex]=i;
		writesector(dir,sectorNumber,0,0);
		/*dirindex++;
		sectcount++;

		//move to the sector and write to it
		readsector(dir,i,0,0);
		for (i=0; i<512; i++)
		{
			if (feof(loadFil))
			{
				fputc(0x0, floppy);
				break;
			}
			else
			{
				char c = fgetc(loadFil);
				fputc(c, floppy);
			}	
		}
	

	//write the map and directory back to the floppy image
        fseek(floppy,512,SEEK_SET);
        for(i=0; i<512; i++)
		fputc(map[i],floppy);
        
        fseek(floppy,512*2,SEEK_SET);
        for (i=0; i<512; i++)
		fputc(dir[i],floppy);

	fclose(floppy);
	fclose(loadFil);
}*/

}