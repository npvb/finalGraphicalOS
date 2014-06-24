/*BASIC FS
WRITE BLOCK
READ BLOCK*/

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