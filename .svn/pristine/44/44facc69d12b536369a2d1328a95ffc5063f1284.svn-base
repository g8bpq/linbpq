// FormatHTML.cpp : Defines the entry point for the console application.
//
#include <stdio.h>

int main () {
   FILE *fp, *fp2;
   char str[256];
   char newstr[256];
   char * ptr, * inptr;

   /* opening file for reading */
   fp = fopen("D:/AtomProject/test.html" , "r");
   fp2 = fopen("D:/AtomProject/test.html.c" , "w");

   if(fp == NULL) {
      perror("Error opening file");
      return(-1);
   }

	while(fgets (str, 256, fp) != NULL)
   {
	   // Replace any " with \" and add " on front and \r\n" on end

	   char c;
	   ptr = newstr;
	   inptr = str;

	   c = *(inptr++);

	   *(ptr++) = '"';

	   while (c && c != 10)
	   {
		   if (c == '"')
			   *(ptr++) = '\\';

		   *(ptr++) = c;

		   c = *(inptr++);
	   }

	  
	   *(ptr++) = '\\';
		*(ptr++) = 'r';
		*(ptr++) = '\\';
		*(ptr++) = 'n';
		*(ptr++) = '"';
		*(ptr++) = 10;
		*(ptr++) = 0;
		
		puts(newstr);
		fputs(newstr, fp2);

	}

	fclose(fp);
	fclose(fp2);
   
   return(0);
}