
#define BASEOFFSET 0x40

void setup()
{
	seti();
}

/*computes a mod b*/
int mod(int a, int b)
{
        while(a>=b)
                a=a-b;
        return a;
}

/*computes a / b using integer division*/
int div(int a, int b)  
{               
        int q=0;
        while (q*b<=a)
                q++;
        return q-1;
}               

/*reads a sector into the buffer*/
void readsector(int sectornumber, char* buffer)
{
        int sec,head,cyl;

	/*convert to CHS*/
        sec=mod(sectornumber,0x12)+1;
        head=mod(div(sectornumber,0x12),2);
        cyl=div(sectornumber,0x24);

        readsect(buffer,sec,head,cyl);
}

/*writes buffer to a sector*/
void writesector(int sectornumber, char* buffer)
{
        int sec,head,cyl;

	/*convert to CHS*/
        sec=mod(sectornumber,0x12)+1;
        head=mod(div(sectornumber,0x12),2);
        cyl=div(sectornumber,0x24);

        writesect(buffer,sec,head,cyl);
}

/*prints a character*/
void putchar(char c)
{
	printc(c);
}

/*reads a character*/
char getchar()
{
	return (char)readc();
}

/*prints a string terminated with 0*/
void printstring(char* string)
{
	int21(1,string);
}

/*prints an integer*/
void printnumber(int number)
{
	char num[12];
	char pnum[12];
	int d=1;
	int i=0;
	int j;

	/*convert the number to a string digit by digit*/
	while(i<10)
	{
		num[i]=mod(div(number,d),10)+0x30;
		i++;
		d=d*10;
		if (div(number,d)==0)
			break;
	}

	/*reverse it to read most significant to least significant*/
	j=0;
	for (d=i-1; d>=0; d--)
		pnum[j++]=num[d];
	pnum[j]=0;
	printstring(pnum);
}

/*read a line from the keyboard and put it in buffer*/
void readstring(char* buffer)
{
	int21(2,buffer);
}

/*read the file name[] into buffer[]*/
void readfile(char* name, char* buffer)
{
	int21(3,name,buffer);
}

/*write buffer[] to a file called name[]*/
void writefile(char* name, char* buffer, int sectorlength)
{
	int21(4,name,buffer,sectorlength);
}

/*delete the file name[]*/
void deletefile(char* name)
{
	int21(5,name);
}

/*execute the program*/
void executeprogram(char* buffer, int bytelength)
{
	int21(8,buffer,bytelength);
}

/*execute the program, but don't make the caller wait*/
void executeprogrambackground(char* buffer, int bytelength)
{
	int21(6,buffer,bytelength);
}

/*terminate program - this must be called by the program before ending*/
void exit()
{
	int21(7);
}

/*sets the video to 1 - graphics, 0 - text*/
void setvideo(int mode)
{
	if (mode==0)
		setvideotext();
	else
		setvideographics();
}

/*sets the pixel at columnxrow to color*/
void setpixel(int color, int column, int row)
{
	drawpixel(color,row*320+column);
}

/*sets the cursor position to row, column*/
void setcursor(int row, int column)
{
	interrupt(0x10,2*256,0,0,row*256+column);
}

/*clear the screen*/
void clearscreen()
{
        int r,c;
        for (c=0; c<80; c++)
                for (r=0; r<25; r++)
                {
                        putInMemory(0xB800,(r*80+c)*2,0x20);
                        putInMemory(0xB800,(r*80+c)*2+1,7);
                }
}

/*prints a character at a certain cursor position*/
void setchar(char c, char color, int row, int column)
{
	putInMemory(0xB800,(row*80+column)*2,c);
	putInMemory(0xB800,(row*80+column)*2+1,color);
}

/*prints a string at a certain cursor position*/
void setstring(char* c, char color, int row, int column)
{
	int i=0;
	while(c[i]!=0x0)
	{
		putInMemory(0xB800,(row*80+column+i)*2,c[i]);
		putInMemory(0xB800,(row*80+column+i)*2+1,color);
		i++;
	}
}

/*turns an integer into a string*/
void getnumberstring(char* pnum, int number)
{
	char num[12];
	int d=1;
	int i=0;
	int j;

	/*convert the number to a string digit by digit*/
	while(i<10)
	{
		num[i]=mod(div(number,d),10)+0x30;
		i++;
		d=d*10;
		if (div(number,d)==0)
			break;
	}

	/*reverse it to read most significant to least significant*/
	j=0;
	for (d=i-1; d>=0; d--)
		pnum[j++]=num[d];
	pnum[j]=0;
}

//**FUNCIONES DEL FS NUEVO
void escribircontenido(char* name, char* buffer, int sectorlength, int sectorNumber)
{
	char dirsector[512];
	char mapsector[512];
	int i,j,k,index,sec,cyl,head;

	if (sectorlength>25)
		return;

	readsector(0x040+1,dirsector);
	readsector(0x40,mapsector);

	i = sectorNumber+6;
	index=0;

	for (j=0; j<sectorlength; j++)
	{
		for (k=0; k<256; k++)
		{
			if (mapsector[k]==0)
				break;
		}
		if (k==256)
			break;
		mapsector[k]=0x46;
		dirsector[i]=(char)k;
		i++;
		
		writesector(0x40 + i,buffer+index);

		index=index+512;
		
	}
	dirsector[i]=0;
	
	writesector(0x40,mapsector);
	writesector(0x40+1, dirsector);	
}

void crearArchivo(char* path, char* content, int size)
{

	int i;
	int sect;
	char map[512];
	char dir[512];
	char nombre[6];
	char found = 0;
	char sectorNumber;
	char comparar[6];
	int index;
	int y;
	int sectorLibre = 0;
	int entradaMapa = 0;
	int dirindex = 0;
	int sectcount;
	int b=0;
	int w=0;
	int z = 0;
	char token [10];
	int toks = CountTokens(path);
	readsector(0x40,map);
	readsector(0x40 +1,dir);	
	sectorNumber = 0x01;
	
	for (index=0; index<10; index++)
		token[index]=0x0;
	while(w < toks)
	{
		z = getNextToken(path,token,z);
		found = 0;

		for(index=0;index<5;index++)
			nombre[index] = token[index];

		for (index=0;index<15;index++)
		{
			
			for(y=0;y<5;y++)
			{
				comparar[y] = dir[index*0x20+y];
			}
			
			if(StringCompare(comparar,nombre,6)==1) 
			{
				sectorNumber = dir[index*0x20+6];
				readsector(0x40+sectorNumber,dir);
				w++;
				found = 1;
				break;
			}				
		}

			if(found == 1)
			{
				continue;
			}
			w++;		
	}

		for (index=0;index<15;index++)
		{
			dirindex = index*0x20;
			sectorLibre = dir[dirindex]; //buscamos la primera entrada libre
			if(sectorLibre==0){
				for (index=0; index<6; index++)
					dir[dirindex+i]=0x20;
				for (i=0; i<6; i++)
				{
					if(nombre[i]==0)
					break;
					dir[dirindex+i]=nombre[i];		
				}
				
				break;
			}
		}	

		sect = (div(size,512)+1);
		for(index=0;index<sect;index++)
		{
				for (entradaMapa=0; entradaMapa<256; entradaMapa++)
				if (map[entradaMapa]==0)
					break;
			if (entradaMapa==256)
			{
				printstring("No tiene mas entradas para archivos.\n");
				return;
			}
			
			map[entradaMapa]=0x46;
			dir[dirindex+6+index] = entradaMapa;
			writesector(0x40+entradaMapa,content+(index*512));
		}

		writesector(0x40+sectorNumber,dir);
		writesector(0x40,map);
		return;
	
}

char find(char sectorAnterior, char* path, char* dir, char sectorNumber)
{
	int ind;
	int tok = 0;
	int toks = CountTokens(path);
	int cont=0;
	int y;
	char nombre[6];
	char nombre2[6];
	char token [10];
	char found = 0;
	
	for (ind=0; ind<10; ind++)
		token[ind]=0x0;

	while(cont < toks)
	{
		tok = getNextToken(path,token,tok);
		
		for(ind=0;ind<6;ind++)
		{
			nombre[ind] = token[ind];
		}

		for (ind=0;ind<15;ind++)
		{		
			for(y=0;y<6;y++)
			{
				nombre2[y] = dir[ind*0x20+y]; 
			}
					
			if(StringCompare(nombre2,nombre,6)==1)
			{
				sectorAnterior = sectorNumber;
				sectorNumber = dir[ind*0x20+6];
				found = 1;
				readsector(0x40+sectorNumber,dir);
				break;
			}

		}
		cont++;
	}
	//return sectorAnterior;
	return found;
}

char findSectorNumber(char* path)
{
	int index;
	int tok = 0;
	int toks = CountTokens(path);
	int cont=0;
	int y;
	int x;
	char nombre[6];
	char dir[512];
	char comparar[6];
	char token [10];
	char sectorNumber = 0x01;
	char prevSectorNumber = 0x01;

	readsector(0x40 +1,dir);

	for (index=0; index<10; index++)
		token[index]=0x0;

	while(cont < toks)
	{
		tok = getNextToken(path,token,tok);
		
		for(index =0;index<=5;index++)
		{
			nombre[index] = token[index];
			
		}

		for (x=0;x<15;x++)
		{		
			for(y=0;y<=5;y++)
			{
				comparar[y] = dir[x*0x20+y]; 
			}
		
			if(StringCompare(comparar,nombre,6)==1)
			{
				prevSectorNumber =sectorNumber;
				sectorNumber = dir[x*0x20+6];
				readsector(0x40+sectorNumber,dir);
				break;
			}

		}
		cont++;
	}
	return sectorNumber;
}

int returnIndex(char* path)
{
	int i;
	char r;
		
	//load the disk map
	char map[512];
	char dir[512];
	char nombre[6];
	char found = 0;
	char sectorNumber ;
	char numSectorAnterior;
	char comparar[6];
	int index=0;
	int indexBuff=0;
	int x;
	int n;
	int y;
	int sectorLibre = 0;
	int entradaMapa = 0;
	int dirindex = 0;
	int sectcount;
	int b=0;
	int w=0;
	int z = 0;
	char token [10];
	char buffFile[512];
	int toks = CountTokens(path);//cantidad de subdirectorios
	readsector(0x40,map);
	readsector(0x40 +1,dir);	
	sectorNumber = 0x01;
	numSectorAnterior = 0x01;

	for (b=0; b<10; b++)
		token[b]=0x0;

	while(w < toks)
	{
		z = getNextToken(path,token,z);
		found = 0;
		
		for(n=0;n<=5;n++)
		{
			nombre[n] = token[n];
			
		}

		for (x=0;x<15;x++)
		{		
			for(y=0;y<=5;y++)
			{
				comparar[y] = dir[x*0x20+y]; 
			}
			
			
			if(StringCompare(comparar,nombre,6)==1)
			{
				numSectorAnterior = sectorNumber;
				sectorNumber = dir[x*0x20+6];
				index = sectorNumber;
				readsector(0x40+sectorNumber,dir);
				found = 1;
				break;
			}

		}
		w++;
	}
	return index;
}

char Sifound(char sectorAnterior, char* path, char* dir, char sectorNumber)
{
	int ind;
	int tok = 0;
	int toks = CountTokens(path);
	int cont=0;
	int y;
	char nombre[6];
	char nombre2[6];
	char token [10];
	char found = 0;
	
	for (ind=0; ind<10; ind++)
		token[ind]=0x0;

	while(cont < toks)
	{
		tok = getNextToken(path,token,tok);
		
		for(ind=0;ind<6;ind++)
		{
			nombre[ind] = token[ind];
		}

		for (ind=0;ind<15;ind++)
		{		
			for(y=0;y<6;y++)
			{
				nombre2[y] = dir[ind*0x20+y]; 
			}
					
			if(StringCompare(nombre2,nombre,6)==1)
			{
				sectorAnterior = sectorNumber;
				sectorNumber = dir[ind*0x20+6];
				readsector(0x40+sectorNumber,dir);
				found = 1;
				break;
			}

		}
		cont++;
	}
	return found;
}

void echo(char* path) //para escribir
{
	int i;
	char r;
		
	//load the disk map
	char map[512];
	char dir[512];
	char nombre[6];
	char found;
	char comparar[6];
	char sectorNumber;
	char numSectorAnterior;
	int indice=0;
	int indexBuff=0;
	int x;
	int n;
	int y;
	int pos;
	int sectorLibre = 0;
	int entradaMapa = 0;
	int dirindex = 0;
	int sectcount;
	int b=0;
	int w=0;
	int z = 0;
	char token [10];
	char buffFile[512];
	int toks = CountTokens(path);//cantidad de subdirectorios
	readsector(0x40,map);
	readsector(0x40 +1,dir);	
	sectorNumber = 0x01;
	numSectorAnterior = 0x01;

	/*for (b=0; b<10; b++)
		token[b]=0x0;

	while(w < toks)
	{
		z = getNextToken(path,token,z);
		found = 0;
		
		for(n=0;n<=5;n++)
		{
			nombre[n] = token[n];
			
		}

		for (x=0;x<15;x++)
		{		
			for(y=0;y<=5;y++)
			{
				comparar[y] = dir[x*0x20+y]; //copiando el nombre del directorio
			}
			
			
			if(StringCompare(comparar,nombre,6)==1) //si encontramos el directorio
			{
				numSectorAnterior = sectorNumber;
				sectorNumber = dir[x*0x20+6];
				indice = sectorNumber;
				readsector(BASEOFFSET+sectorNumber,dir);
				found = 1;
				break;
			}

		}
		w++;
	}*/
	printstring("sector number");
	printnumber(sectorNumber);
	printstring("index");
	printnumber(indice);
	sectorNumber = findSectorNumber(path);
	indice = sectorNumber;

	printstring("sector number");
	printnumber(sectorNumber);
	printstring("index");
	printnumber(indice);

	found = Sifound(numSectorAnterior,path, dir, sectorNumber);

	if( found == 1)
	{
		//indice = findSectorNumber(path);
		
		while(dir[indice]!=0x00)
		{
			readsector(0x40+indice,buffFile+indexBuff);			
			indice++;
			indice=indexBuff+512;
		}

		printstring(buffFile);
		
	}else
	{
		printstring("Archivo no Encontrando");
		
	}	
}

void PrintList(char sectorNumber, int depth, char* mapsector)
{
	char dirsector[512];
	char name[7];
	int i,j,index,files,x;
	char symbol [3];
	symbol[0] = '-';
	symbol[1] = '>';
	symbol[2] = 0;

	readsector(0x40+ sectorNumber,dirsector);
	index=0;
	files=0;

	for (i=0; i<16; i++)
	{
		/*check if the file exists*/
		if (dirsector[index]!=0)
		{
			/*print hte name*/
			for (j=0; j<6; j++)
				name[j]=dirsector[index+j];
			name[6]=0;
			for(x =0;x<depth;x++)
				printstring("\t");
			printstring(symbol);
			printstring(name);
			sectorNumber = dirsector[i*0x20+6];
			if(mapsector[sectorNumber] == 0x44)
			{	

				j=0;
				while(dirsector[index+j+6]!=0)
					j++;

				printstring("  \0");
				printstring( "D\0");
				printstring("\r\n\0");
				PrintList(sectorNumber,depth+1,mapsector);
				files++;
			}
			else if(mapsector[sectorNumber] == 0x46)
			{	

				/*calculate the file size in sectors from the directory entry*/
				j=0;
				while(dirsector[index+j+6]!=0)
					j++;
				printstring(".txt");
				printstring("  \0");
				printstring("F\0");
				printstring("\r\n\0");
				//PrintList(sectorNumber,depth+1,mapsector);
				files++;
			}
		}
		/*move to the next entry*/
		index=index+0x20;
	}
}

char findSubDir(char* path, char* dir)
{ 	
	int index;
	int lon = 0;
	int lasttokenindex = 0;
	int y;
	int toks = CountTokens(path);
	char token [10];
	char nombre[6];
	char nombre2[6];
	char sectorNumber=0x01;

	for (index=0; index<10; index++)
		token[index]=0x0;
	while(lon  < toks)
	{
		lasttokenindex = getNextToken(path,token,lasttokenindex);
		
		for(index=0;index<6;index++)
		{
			nombre[index] = token[index];
		}

		for (index=0;index<15;index++)
		{
			
			for(y=0;y<6;y++)
			{
				nombre2[y] = dir[index*0x20+y]; 
			}
			
			if(StringCompare(nombre2,nombre,6)==1) 
			{
				sectorNumber = dir[index*0x20+6];
				readsector(0x40+sectorNumber,dir);
				lon ++;
			}
				
		}
		lon ++;
	}
	return sectorNumber;
}

void List(char* path)
{
	char mapsector[512];
	char dir[512];
	char sectornumber;
	readsector(0x40,mapsector);
	readsector(0x40+1,dir);
	printstring("Dir:\r\n\0");
	sectornumber = findSubDir(path,dir);
	PrintList(sectornumber,0,mapsector);	
}

void remove(char sectorNumber, char* map)
{
	char directory[512];
	int x;
	int numBloque;

	readsector(0x40+sectorNumber,directory);
	for (x=0;x<15;x+=0x20)
	{	
		if(directory[x] != 0)
		{
			for(numBloque = 6; numBloque<0x20;numBloque++)
			{
				if(map[directory[x+numBloque]]==0x44)
				{
					remove(directory[x+numBloque],map);
					break;						
				}
				else if(map[directory[x+numBloque]]==0x46)
				{
					remove(directory[x+numBloque],map);
					break;						
				}

			}
		}			
	}
	map[sectorNumber] = 0x0;
	for(x=0;x<512;x++)
	{
		directory[x] = 0x0;
	}

	writesector(0x40+sectorNumber,directory);
	writesector(0x40,map);
}

void rm(char* path)
{
	char map[512];
	char dir[512];
	char nombre[6];
	char found = 0;
	char sectorNumber;
	char prevSectorNumber;
	char comparar[6];
	int x;
	int y;
	int sectorLibre = 0;
	int entradaMapa = 0;
	int dirindex = 0;
	int sectcount;
	int index=0;
	int count=0;
	int z = 0;
	char token [10];
	int toks = CountTokens(path);
	readsector(BASEOFFSET,map);
	readsector(BASEOFFSET +1,dir);	
	sectorNumber = 0x01;
	prevSectorNumber = 0x01;

	for (index=0; index<10; index++)
		token[index]=0x0;
	while(count < toks)
	{
		z = getNextToken(path,token,z);
		found = 0;
		
		for(index=0;index<=5;index++)
		{
			nombre[index] = token[index];
			
		}

		for (x=0;x<15;x++)
		{		
			for(y=0;y<=5;y++)
			{
				comparar[y] = dir[x*0x20+y]; 
			}
				
			if(StringCompare(comparar,nombre,6)==1)
			{
				prevSectorNumber = sectorNumber;
				sectorNumber = dir[x*0x20+6];
				readsector(BASEOFFSET+sectorNumber,dir);
				found = 1;
				break;
			}

		}
		count++;
	}
	
	/*pos = posArchivoEncontrado(path);
	sectorNumber = findSectorNumber(path);*/

	if(found == 1)
	{
		remove(sectorNumber,map);
		readsector(BASEOFFSET + prevSectorNumber,dir);

		for(y=0;y<7;y++)
		{
			dir[x*0x20+y] = 0;
		}
	
		writesector(BASEOFFSET + prevSectorNumber,dir);
	
	}else
	{
		printstring("Directorio no Encontrando");
	}		
}

void createFolder(char* name, char sectorNumber)
{
	int i;
	char r;
		
	//load the disk map
	char map[512];
	char dir[512];
	char nombre[6];
	char found = 0;
	int x;
	int n;
	int y;
	int dirindex;
	int sectcount;
	
	//Cargamos el directorio donde se va escribir como directorio actual
	
	readsector(BASEOFFSET,map);
	readsector(BASEOFFSET+sectorNumber,dir);
	//Encontrando bloque libre
	for (i=0; i<512; i=i+0x20){		
		if (dir[i]==0){
			break;
		}
	}
	
	dirindex=i;
	

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
	sectcount=0;

		if (sectcount==26)
		{
			return;
		}

		dir[dirindex]=i;
		writesector(BASEOFFSET+sectorNumber,dir);
		writesector(BASEOFFSET,map);
}

void mkdir(char* path)
{

	char map[512];
	char dir[512];
	char nombre[6];
	char found = 0;
	char sectorNumber;
	char comparar[6];
	int x;
	int index;
	int y;
	int sectorLibre = 0;
	int mapEntry = 0;
	int dirindex = 0;
	int sectcount;
	int count=0;
	int tok = 0;
	char token [10];
	int toks = CountTokens(path);
	readsector(BASEOFFSET,map);
	readsector(BASEOFFSET +1,dir);	
	sectorNumber = 0x01;
	
	for (index=0; index<10; index++)
		token[index]=0x0;
	while(count < toks)
	{
		tok = getNextToken(path,token,tok);
		found = 0;
		
		for(index=0;index<6;index++)
		{
			nombre[index] = token[index];
			
		}
		for (index=0;index<15;index++)
		{
			
			for(y=0;y<6;y++)
			{
				comparar[y] = dir[index*0x20+y]; 
			}
			
			if(StringCompare(comparar,nombre,6)==1) 
			{
				sectorNumber = dir[index*0x20+6];
				readsector(BASEOFFSET+sectorNumber,dir);
				count++;
				found = 1;
				break;
			}
					
		}

		if(found == 1)
		{
			continue;
		}		
		
		for (mapEntry=0;mapEntry<256;mapEntry++)
			if (map[mapEntry]==0)
				break;
		if (mapEntry==256)
		{
			printstring("NO hay mas espacio para directorios\n");
			return;
		}
		//mark the map entry as taken
		map[mapEntry]=0x44;

		for (x=0;x<15;x++)
		{
			dirindex = x*0x20;
			sectorLibre = dir[dirindex]; //buscamos la primera entrada libre
			if(sectorLibre==0){
				for (index=0; index<6; index++)
					dir[dirindex+index]=0x20;
				for (index=0; index<6; index++)
				{
					if(nombre[index]==0)
					break;
					dir[dirindex+index]=nombre[index];		
				}
				dir[dirindex+6] = mapEntry;
				break;
			}
		}

		writesector(BASEOFFSET+sectorNumber,dir);
		writesector(BASEOFFSET,map);
		readsector(BASEOFFSET+mapEntry,dir);
		sectorNumber = mapEntry;
		count++;

	}
}

void Format()
{
	int i =0;
	char arreglo [512];
	for(i=0; i<512;i++)
	{
		arreglo[i]=0;
	}

	for(i=0; i<512;i++)
	{
		writesector(0X40+i,arreglo); 
	}

	arreglo[0] = 'M';
	arreglo[1] = 'D';

	writesector(0X40,arreglo);

	printstring("Formateado");
}

int StringCompare(char* nombre, char* name, int size)
{

	int i=0;
	int retonar = 1;

	for(i=0;i<size;i++)
	{
		 if(nombre[i]!=name[i])
			return 0;
	}

	return retonar;
}

int CountTokens(char* path)
{
	int count =0, i=0;
	if(path[0]=='/')
		i++;

	while(path[i] != 0x0)
	{
		
		if(path[i] == '/')
			count++;
		
		i++;
	}
	if(path[i-1]!='/')
		count++;
	return count;
}

int getNextToken(char* path, char* buffer, int next)
{
	int i = 0;
	int j=0;
	
	for(i =0; i<10; i++)
		buffer[i] = 0x20;
	for(i=next;i<10;i++)
	{
		if(path[i]=='/' || path[i] == 0)
			break;
		buffer[j]=path[i];
		//buffer[j+1] = 0x0;
		j++;		
	}
	
	return i+1;
}

void strTok(char* path)
{

	int y = CountTokens(path);
	int i=0;
	int x=0,w=0;
	int z = 0;
	char token [10];

	for (i=0; i<10; i++)
		token[i]=0x0;
	while(w < y)
	{
		z = getNextToken(path,token,z);
		//z=x;
		w++;

	}
}


