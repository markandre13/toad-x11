/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <stdio.h>

int main(int argc, char **argv)
{
	int i, x, byte;
	unsigned size;
	FILE *in;

#if 0
	if (argc==1) {
		printf("TOAD binary to C converter 1.1\n");
		return 0;
	}
#endif

	printf("/* Generated automatically from various files by bin2c. */\n"
				 "\n"
				 "#include <toad/io/urlstream.hh>\n"
				 "using namespace toad;\n"
				 "\n"
				 "void createMemoryFiles()\n"
				 "{\n");
	
	for(i=1; i<argc; i++) {
		in = fopen(argv[i], "r");
		if (!in) {
			perror(argv[i]);
			return 1;
		}
		
		printf("  /* file: %s */\n", argv[i]);
		printf("  static const char data%04i[] = {\n", i);
				
		size = 0;
		x = 0;
		while(!feof(in)) {
			if (x==0) {
				printf("    ");
				x+=4;
			}
			byte = fgetc(in);
			if (byte==-1)
				break;
			printf("0x%02x, ", byte);
			x+=6;
			if (x>78-6) {
				printf("\n");
				x=0;
			}
			size++;
		}
		fclose(in);
		
		printf("\n  };\n");
		printf("  urlstreambase::saveMemoryFile(\"%s\", data%04i, %u);\n\n",
			argv[i], i, size);
	}
	
	printf("}\n");
	
	return 0;
}
