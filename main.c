/*
 Acer BeTouch E130 Android ROM Update Tool for Linux
 Copyright (C) 2011 Yohanes Nugroho

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 Yohanes Nugroho (yohanes@gmail.com)
 Chiang Mai Thailand
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "rut.h"
#include "nb0.h"
#include "mlf.h"


const char * license =
	"\n Acer BeTouch E130 Android ROM Update Tool for Linux\n\n"
	"Copyright (c) 2011 Yohanes Nugroho\n"
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation; either version 2 of the License, or\n"
	"(at your option) any later version.\n\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n\n"

	"You should have received a copy of the GNU General Public License\n"
	"along with this program; if not, write to the Free Software\n"
	"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\n";

int print_license()
{
	printf("%s", license);
	return 0;
}

int print_usage()
{
	printf(	"Linux-RUT ROM Update Tools for Linux\n"
		"Copyright (c) 2011 Yohanes Nugroho\n\n");
	printf("Usage: \n"
	       "  rut l\n"
	       "\tPrint license (GPL)\n\n"
	       "  rut t nb0file\n"
	       "\tTest and list content of nb0 file\n\n"
	       "  rut x nb0file directory\n"
	       "\tExtract nb0file to directory\n\n"
	       "  rut c nb0file directory\n"
	       "\tCreate nb0file from files in directory\n\n"
	       "  rut w [options] nb0file\n"
	       "\twrite entire nb0 file to flash\n\n"
	       "  rut w [options] nb0file package_name\n"
	       "\twrite package_name in nb0 file to flash\n\n"
	       "Options for w:\n"
	       "  -v vendorid\n"
	       "\tVendor id (in hex), for example -v 0x502\n\n"
	       "  -p productid\n"
	       "\tProduct id (in hex), for example -p 0x3214\n\n"
	       "  -i number\n"
	       "\tInput endpoint (default 2)\n\n"
	       "  -o number\n"
	       "\tOutput endpoint (default 1)\n\n"
	       "  -d\n"
	       "\tVerbose output (debug)\n\n"
	       );
	return 0;
}

int extract_nb0_file(int argc, char *argv[])
{
	int r;
	if (argc<4)
		return print_usage();
	r = mkdir(argv[3],  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	if (r!=0 && errno!=EEXIST) {
		fprintf(stderr, "ERROR creating directory '%s'\n", argv[3]);
		return -1;
	}
	if (extract_nb0(argv[2], argv[3])!=0) {
		fprintf(stderr, "ERROR: failed extracting nb0 file\n");
		return -1;
	}

	return 0;
}

int create_nb0_file(int argc, char *argv[])
{
	if (argc<4)
		return print_usage();

	if (create_nb0(argv[2], argv[3])!=0) {
		fprintf(stderr, "ERROR failed creating nb0 file\n");
		return -1;
	} else {
		printf("%s created successfully\n", argv[2]);
	}

	return 0;
}

int test_nb0_file(int argc, char *argv[])
{
	int i;

	if (argc<3)
		return print_usage();

	extract_nb0(argv[2], 0);
	for (i=0; i < nb0_count; i++) {
		char *name = nb0_headers[i].name;
		printf("Package: %s (offset %08lx, %ld bytes)\n", 
		       name, 
		       nb0_headers[i].offset, 
		       nb0_headers[i].size);
	}

	free_nb0();

	return 0;
}

int main(int argc, char *argv[])
{

	if (argc<2 || strlen(argv[1])!=1) {
		return print_usage();
	}

	switch (argv[1][0]) {
	case 'l':
		return print_license();
	case 'x':
		return extract_nb0_file(argc, argv);
	case 'c':
		return create_nb0_file(argc, argv);
	case 'w':
		return write_nb0_file(argc, argv);
	case 't':
		return test_nb0_file(argc, argv);
	default:
		return print_usage();

	}
}
