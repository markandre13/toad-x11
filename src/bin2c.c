/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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
#include <string.h>
#include <stdlib.h>

static const char * prgname = "TOAD C++ GUI Library: Binary to C converter 1.2\n";

static void 
usage()
{
  printf(
    "%s"
    "Usage: toadbin2c [OPTION] FILE ...\n"
    "\n"
    "Valid command line options are:\n"
    "--function-name <name>\n"
    "    specifies the function name used to initialize the memory files\n"
    "    default value is 'createMemoryFiles'\n"
    "--prefix <prefix>\n"
    "    an additional prefix for the memory filenames\n"
    "--\n"
    "    last option, all following arguments are treated as files\n"
    "--help\n"
    "    display this help and exit\n"
    "\n"
    "Please report bugs or feedback to <toad-bug@mark13.de>\n",
    prgname
  );
  exit(0);
}

int
main(int argc, char **argv)
{
	int i, j, x, byte;
	unsigned size;
	FILE *in;
	
	char * functionname = "createMemoryFiles";
	char * prefix       = "";

  for(i=1; i<argc; ++i) {
    if (strcmp(argv[i], "--")==0) {
      ++i;
      break;
    }
    if (strcmp(argv[i], "--help")==0) {
      usage();
    } else
    if (strcmp(argv[i], "--function-name")==0) {
      if (argc-i<1) {
        fprintf(stderr, "not enough arguments for %s\n", argv[i]);
        return EXIT_FAILURE;
      }
      functionname = argv[++i];
    } else
    if (strcmp(argv[i], "--prefix")==0) {
      if (argc-i<1) {
        fprintf(stderr, "not enough arguments for %s\n", argv[i]);
        return EXIT_FAILURE;
      }
      prefix = argv[++i];
    } else
      break;
  }

	printf("/* Generated automatically from various files by toadbin2c. */\n"
				 "\n"
				 "#include <toad/io/urlstream.hh>\n"
				 "using namespace toad;\n"
				 "\n"
				 "void %s()\n"
				 "{\n", functionname);

	for(j=0; i<argc; i++, j++) {
		in = fopen(argv[i], "r");
		if (!in) {
			perror(argv[i]);
			return EXIT_FAILURE;
		}
		
		printf("  /* file: %s%s */\n", prefix, argv[i]);
		printf("  static const char data%04i[] = {\n", j);
				
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
		printf("  urlstreambase::saveMemoryFile(\"%s%s\", data%04i, %u);\n\n",
			prefix, argv[i], j, size);
	}
	
	printf("}\n");
	
	return EXIT_SUCCESS;
}
