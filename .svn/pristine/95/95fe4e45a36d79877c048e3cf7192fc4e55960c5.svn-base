/*
Copyright 2001-2022 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/	

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#include "compatbits.h"
#include <string.h>

#ifndef WIN32

#define APIENTRY
#define DllExport
#define VOID void

#else
#include <windows.h>
#endif

void * zalloc(int len);

/* jer:
 * This is the original file, my mods were only to change the name/semantics on the b64decode function
 * and remove some dependencies.
 */
/*
	LibCGI base64 manipulation functions is extremly based on the work of Bob Tower,
	from its projec http://base64.sourceforge.net. The functions were a bit modicated. 
	Above is the MIT license from b64.c original code:

LICENCE:        Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

                Permission is hereby granted, free of charge, to any person
                obtaining a copy of this software and associated
                documentation files (the "Software"), to deal in the
                Software without restriction, including without limitation
                the rights to use, copy, modify, merge, publish, distribute,
                sublicense, and/or sell copies of the Software, and to
                permit persons to whom the Software is furnished to do so,
                subject to the following conditions:

                The above copyright notice and this permission notice shall
                be included in all copies or substantial portions of the
                Software.

                THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
                KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
                WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
                PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
                OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
                OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
                OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
                SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

*/

static char mycd64[256] = "";
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // / causes problems with freedata

void xencodeblock( unsigned char in[3], unsigned char out[4], int len )
{
	if (mycd64[0] == 0)
	{
		int i,j;

		for (i=0;i<64; i++)
		{
			j=cb64[i];
			mycd64[j]=i;
		}
	}

    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

void xdecodeblock( unsigned char in[4], unsigned char out[3] )
{   
    char Block[5];
	int i,j;

	for (i=0;i<64; i++)
	{
		j=cb64[i];
		mycd64[j]=i;
	}


	Block[0]=mycd64[in[0]];
    Block[1]=mycd64[in[1]];
    Block[2]=mycd64[in[2]];
    Block[3]=mycd64[in[3]];

	out[0] = (unsigned char ) (Block[0] << 2 | Block[1] >> 4);
    out[1] = (unsigned char ) (Block[1] << 4 | Block[2] >> 2);
    out[2] = (unsigned char ) (((Block[2] << 6) & 0xc0) | Block[3]);
}

/** 
* @ingroup libcgi_string
* @{
*/

/**
* Encodes a given tring to its base64 form.
* 
* @param *str String to convert
* @return Base64 encoded String
* @see str_base64_decode
**/
char * xstr_base64_encode(char *str)
{
    unsigned int i = 0, j = 0, len = (int)strlen(str);
	char *tmp = str;
	char *result = (char *)zalloc((len+1) * sizeof(void *));
	
	if (!result)
		return NULL;

	while (len  > 2 )
	{
		xencodeblock(&str[i], &result[j],3);
		i+=3;
		j+=4;
		len -=3;
	}
	if (len)
	{
		xencodeblock(&str[i], &result[j], len);
	}

	return result;
}

char * byte_base64_encode(char *str, int len)
{
    unsigned int i = 0, j = 0;
	char *tmp = str;
	char *result = (char *)zalloc((len * 2 )+ 5);
	
	if (!result)
		return NULL;

	while (len > 2 )
	{
		xencodeblock(&str[i], &result[j],3);
		i+=3;
		j+=4;
		len -=3;
	}
	if (len)
	{
		xencodeblock(&str[i], &result[j], len);
	}

	return result;
}

