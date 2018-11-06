#include <stdio.h>
#include <string.h>
FILE *fp;
char buffer[1080*96*1735];

int main () {


   /* Open file for both reading and writing */

   fp = fopen("625614315Cube.raw", "rb");


   fseek(fp, 0, SEEK_SET);
   /* Read and display data */
   fread(buffer,sizeof(char), (1080*96*1735+1), fp);
   printf("%s\n", buffer);
   fclose(fp);

   return(0);
}
