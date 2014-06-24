
char *strcopy(char * destination, const char * source)
{
  char *destination_ptr;
  destination_ptr = destination;
   
  while(*destination++=*source++);
    return(destination_ptr);

}

char** strtok(char* path, char token)
{

}

int StringCompare(char* nombre, char* name, int size)
{

	int i=0;
	int retonar = 1;

	printstring("string compare:\r\n\0");
	for(i=0;i<size;i++)
	{
		 if(nombre[i]!=name[i])
			return 0;
	}

	return retonar;
}