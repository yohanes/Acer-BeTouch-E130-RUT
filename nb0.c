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

#include "nb0.h"
#include "util.h"

int nb0_count;
nb0_header_item *nb0_headers;

static long read_dword(FILE *f)
{
	int a,b,c,d;
	long result;
	a = getc(f);
	b = getc(f);
	c = getc(f);
	d = getc(f);
	result = a | b << 8 | c << 16 | d << 24;
	return result;	
}

void write_dword(FILE *f, long l)
{
	fputc(l & 0xff, f);
	fputc((l >> 8) & 0xff, f);
	fputc((l >> 16) & 0xff, f);
	fputc((l >> 24) & 0xff, f);	
}


static int read_header(nb0_header_item *header, FILE *f)
{
	int i = 0;

	header->offset = read_dword(f);
	header->size = read_dword(f);
	//ignore these
	read_dword(f);
	read_dword(f);
	fread(header->name, 48, 1, f);
	header->name[48] = '\0';
	i = 48;
	while (i>=0) {
		if (header->name[i]==' ') {
			header->name[i]='\0';
		} else {
			break;
		}
	}

	return 0;

}

static char buffer[1024*1024];

int extract_nb0(const char *filename, const char *extract_to)
{
	char buf[FILENAME_MAX];
	FILE *f;
	FILE *listfile = 0;
	int i;
	int size;
	long fsize;
	long lastoffset;

	f = fopen(filename, "r");

	if (!f) {
		fprintf(stderr, "error opering file %s\n", filename);
		return -1;
	}

	if (extract_to) {
		sprintf(buf, "%s/%s", extract_to, "list");
		listfile = fopen(buf,  "w");
		if (!listfile) {
			fprintf(stderr, "ERROR can not create 'list' file\n");
			fclose(f);
			return -1;
		}
	}	

	fseek(f, 0, SEEK_END);

	fsize = ftell(f);

	rewind(f);
	
	size = read_dword(f);


	if (size>(fsize/64)) {
		fprintf(stderr, "ERROR invalid nb0 file\n");
		fclose(f);
		return -1;
	}

	nb0_count = size;

	printf("File count: %d\n", size);

	nb0_headers = (nb0_header_item *)malloc(sizeof(nb0_header_item)*size);


	lastoffset = 0;

	for (i = 0; i < size; i++) {
		nb0_header_item *nb;
		read_header(&nb0_headers[i], f);		
		nb = &nb0_headers[i];

		if (feof(f) || (nb->offset < lastoffset)) {
			fprintf(stderr, "ERROR invalid nb0 file\n");
			free_nb0();
			fclose(f);
			return -1;
		}

		//printf("offset = %08lx size = %08lx name: '%s'\n", nb->offset, nb->size, nb->name);
	}

	fseek(f, 64*size + 4, SEEK_SET);

	//printf("filepos %ld\n", ftell(f));

	for (i = 0; i < size; i++) {
		int sz;
		FILE *out = 0;
		nb0_header_item *nb = &nb0_headers[i];
		nb->nb0_file_offset = ftell(f);

		if (extract_to) {
			sprintf(buf, "%s/%s", extract_to, nb->name);
			printf("extracting to %s (%ld bytes)\n", buf, nb->size);
			fprintf(listfile, "%s\n", nb->name);
			out = fopen(buf, "w");
			if (!out) {
				fclose(f);
				fprintf(stderr, "ERROR can not create output file '%s'\n", buf);
				free_nb0();
				return -1;
			}
		}
		/*we will read the whole file even though we don't
		  extract it.  This is to make sure that we can really
		  read the file.
		 */
		
		sz = nb->size;

		while (sz>0) {

			int toread, rd;

			if (feof(f)) {
				fclose(f);
				fprintf(stderr, "ERROR unexpected end of file\n");
				free_nb0();
				if (listfile) 
					fclose(listfile);
				return -1;
			}

			toread = sz > sizeof(buffer)?sizeof(buffer):sz;

			rd = fread(buffer, 1, toread, f);

			if (rd<=0)  {
				if (ferror(f)) {
					fclose(f);
					if (listfile) 
						fclose(listfile);
					return -1;
				}
				break;
			}			

			sz -= rd;

			if (extract_to) {
				fwrite(buffer, 1, rd, out);
			}
		}
		if (out)
			fclose(out);
	}

	fclose(f);
	if (listfile)
		fclose(listfile);

	return 0;
}

void free_nb0()
{
	free(nb0_headers);
	nb0_count = 0;
	nb0_headers = 0;
}


int create_nb0(const char *outfilename, const char *from_dir)
{
	int i;
	char buf[FILENAME_MAX];
	char filename[FILENAME_MAX];
	FILE *outfile;
	FILE *infile;
	FILE *listfile = 0;
	int filename_count = 0;
	char **filename_list = 0;
	long *file_sizes = 0;
	long offset = 0;
	int err = 0;

	sprintf(buf, "%s/list", from_dir);       

	listfile = fopen(buf, "r");
	if (!listfile) {
		fprintf(stderr, "ERROR can not open '%s'\n", buf);
		return -1;
	}

	while (!feof(listfile)) {
		if (fgets(filename, sizeof(filename), listfile)==0)
			break;
		trim(filename);

		if (strlen(filename)>48) {
			fprintf(stderr, "ERROR filename '%s' is too long\n",
				filename);
			for (i = 0; i < filename_count; i++) 
				free(filename_list[i]);
			free(filename_list);
			return -1;
		}

		filename_list = (char **)realloc(filename_list, sizeof(char *)*
						 (filename_count+1));
		filename_list[filename_count] = strdup(filename);
		filename_count++;
	}
	
	file_sizes = (long *)malloc(filename_count*sizeof(long));

	for (i = 0; i < filename_count; i++) {
		sprintf(buf, "%s/%s", from_dir, filename_list[i]);
		infile = fopen(buf, "r");
		if (!infile) {
			fprintf(stderr, "ERROR can not open file '%s' \n", 
				buf);
			err = -1;
			goto free_list;
		}
		fseek(infile, 0, SEEK_END);
		file_sizes[i] = ftell(infile);
		fclose(infile);
	}

	outfile = fopen(outfilename, "w");

	if (!outfile) {
		err = -1;
		fprintf(stderr, "ERROR: can not open output file\n");
		goto free_list;
	}

	write_dword(outfile, filename_count);

	offset = 0;

	for (i = 0; i < filename_count; i++)  {
		write_dword(outfile, offset);
		write_dword(outfile, file_sizes[i]);
		offset += file_sizes[i];
		write_dword(outfile, 0);
		write_dword(outfile, 0);
		memset(filename, 0, sizeof(filename));
		strcpy(filename, filename_list[i]);
		fwrite(filename, 1, 48, outfile);
	}

	for (i = 0; i < filename_count; i++)  {
		sprintf(buf, "%s/%s", from_dir, filename_list[i]);

		printf("Adding : %s\n", buf);
		infile = fopen(buf, "r");
		if (!infile) {
			fprintf(stderr, "ERROR can not open file '%s' \n", 
				buf);
			err = -1;
			fclose(outfile);
			goto free_list;
		}
		while (!feof(infile) && !ferror(infile)) {
			int rd = fread(buffer, 1, sizeof(buffer), infile);
			if (rd<=0)
				break;
			if (fwrite(buffer, 1, rd, outfile)!=rd) {
				fprintf(stderr, "ERROR can write to nb0 file\n");
				err = -1;
				fclose(infile);
				fclose(outfile);
				goto free_list;				
			}
		}

		fclose(infile);
	}

	fclose(outfile);

	free_list:	
	for (i = 0; i < filename_count; i++) 
		free(filename_list[i]);
	free(filename_list);
	free(file_sizes);
	return err;
}


/* static int test_main_nb0(int argc, char *argv[]) */
/* { */
/* 	extract_nb0("/Users/tc17/Downloads/kernel-baru-ext3-compcache.nb0",  */
/* 		 "/Users/tc17/extract"); */
/* 	free_nb0(); */
/* 	return 0; */
/* } */

