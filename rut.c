/*
 Acer BeTouch E130 Android ROM Update Tools for Linux
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

#include <libusb-1.0/libusb.h>

#include "nb0.h"
#include "mlf.h"
#include "util.h"

/**
 * Acer BeTouch E130
 */
#define VENDOR 0x0502
#define PRODUCT 0x3214
//#define TRANSFER_SIZE 65536

#define TRANSFER_SIZE 0x200

//#define SIMULATE

static int vendor_id  = VENDOR;
static int product_id  = PRODUCT;
static int in_endpoint = 2;
static int out_endpoint = 1;
static int verbose = 0;

static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)\n",
		       desc.idVendor, desc.idProduct,
		       libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}

static struct libusb_device_handle *devh = NULL;


static int find_device(void)
{
	
	if (verbose) {
		printf("Looking for %04x:%04x\n", vendor_id, product_id);		
	}
	
	devh = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
	return devh ? 0 : -1;
}




#ifdef SIMULATE

int usb_out(int count, unsigned char *data)
{
	return count;
}


int usb_in(int count, unsigned char *data)
{
	return count;
}


#else


static int usb_out(int count, unsigned char *data)
{
	int actual_length;

	int r =  libusb_bulk_transfer(devh,
				      out_endpoint|LIBUSB_ENDPOINT_OUT,
				      data, count, &actual_length, 0);

	if (r!=0) {
		fprintf(stderr, "USB ERROR: %d\n", r);
		return -1;
	}
	return actual_length;
}


static int usb_in(int count, unsigned char *data)
{
	int r;
	int actual_length;

	memset(data, 0, count);

	r = libusb_bulk_transfer(devh,
				 in_endpoint|LIBUSB_ENDPOINT_IN,
				 data, count, &actual_length, 0);

	if (r!=0) {
		fprintf(stderr, "USB ERROR: %d\n", r);
		return -1;
	}

	//printf("result length: %d\n", actual_length);
	return actual_length;

}

#endif


static void hexdump(const unsigned char *data, int len)
{
	int i;
	for (i=0; i<len; i++) {
		printf("%02x ", data[i]);
		if (((i+1)%16)==0)
			printf("\n");
	}
	printf("\n");
}


static unsigned char file_buf[0x84000];

typedef int(*readfunction)(void *read_data, unsigned char *buffer, int size);
typedef void(*donefunction)(void *read_data);

typedef struct {
	int offset;
	int erase_size;
	int partition_id;
	int file_size;
	void *read_data;
	readfunction read;
	donefunction done;
} data_provider;



static int send_to_device(data_provider *data_prov)
{
	unsigned char data[14];
	long fsize;
	long erasesize;
	long offset;
	int partition_id;

	fsize = data_prov->file_size;
	erasesize = data_prov->erase_size;
	offset = data_prov->offset;
	partition_id = data_prov->partition_id;

        printf("Sending: %ld bytes (erase size = %ld bytes) partition id = %d offset = 0x%08lx\n",
	       fsize, erasesize, partition_id, offset);


	//ERASE COMMAND
	data[0] = 0x31;

	data[1] = offset & 0xff;
	data[2] = (offset >>8)  & 0xff;
	data[3] = (offset >>16)  & 0xff;
	data[4] = (offset >>24)  & 0xff;

	data[5] = fsize & 0xff;
	data[6] = (fsize >>8)  & 0xff;
	data[7] = (fsize >>16)  & 0xff;
	data[8] = (fsize >>24)  & 0xff;

	data[9] = erasesize & 0xff;
	data[10] = (erasesize >>8)  & 0xff;
	data[11] = (erasesize >>16)  & 0xff;
	data[12] = (erasesize >>24)  & 0xff;

	data[13] = partition_id;

	if (verbose) {
		printf("Sending ERASE:" );
		hexdump(data, 14);
	}

	if (usb_out(14, data)!=14) {
		fprintf(stderr, "error sending ERASE\n");
		data_prov->done(data_prov->read_data);
		return -1;
	}

	if (usb_in(4, data)!=4) {
		fprintf(stderr, "error receiving ERASE response\n");
		data_prov->done(data_prov->read_data);
		return -1;
	}

	if (verbose) {
		fprintf(stderr, "return value of erase: " );
		hexdump(data, 4);
	}

	int firstsend = 1;

	int to_send = fsize;

	int max_size = partition_id==1?0x80000:0x84000;

	if (verbose)
		printf("partition: %d max_size %d\n", partition_id, max_size);


	while (1) {
		int rd = data_prov->read(data_prov->read_data, file_buf, max_size);
		if (rd<=0)
			break;

		to_send -= rd;

		//WRITE COMMAND

		data[0] = 0x51;

		if (firstsend) {
			//OFFSET
			data[1] = offset & 0xff;
			data[2] = (offset >>8)  & 0xff;
			data[3] = (offset >>16)  & 0xff;
			data[4] = (offset >>24)  & 0xff;
		} else {
			//CONTINUE
			data[1] = 1;
			data[2] = 0;
			data[3] = 0;
			data[4] = 0;
		}

		//SIZE
		data[5] = rd & 0xff;
		data[6] = (rd >>8)  & 0xff;
		data[7] = (rd >>16)  & 0xff;
		data[8] = (rd >>24)  & 0xff;
		data[9] = partition_id;

		if (verbose) {
			printf("Sending prepare write: ");
			hexdump(data, 10);
		}

		if (usb_out(10, data)!=10) {
			fprintf(stderr, "ERROR sending prepare write\n");
			data_prov->done(data_prov->read_data);
			return -1;
		}

		if (usb_in(1, data)!=1) { 	//x51 or x57
			fprintf(stderr, "ERROR receiving prepare write\n");
			data_prov->done(data_prov->read_data);
			return -1;
		}

		if (verbose) {
			printf("return value of prepare write: " );
			hexdump(data, 1);
		}

#ifndef SIMULATE
		if (data[0]!=0x51 && data[0]!=0x57) {
			printf("WARNING: response is %02x\n", data[0]);
		}
#endif

		int error = 0;

		printf(".");fflush(stdout);

		int data_offset = 0;

		while (rd>0) {
			int tr = rd>=TRANSFER_SIZE?TRANSFER_SIZE:rd;

			int r = usb_out(tr, file_buf + data_offset);
			if (r<0) {
				fprintf(stderr, "ERROR transfering data\n");
				error = 1;
				break;
			}
			rd -= r;
			data_offset += r;

		}
		if (error)
			break;


		if (to_send>0 && !firstsend) {

			if (usb_in(4, data)!=4) {
				fprintf(stderr, "error receiving progress report\n");
				return -1;
			}
			if (verbose) {
				printf("progress: ");
				hexdump(data, 4);
			}
		}

		if (firstsend)
			firstsend = 0;


	}

	//should be just a warning for the last one
	if (usb_in(4, data)!=4) {
		printf("warning: not receiving progress report\n");
	} else {
		if (verbose) {
			printf("progress: ");
			hexdump(data, 4);
		}
	}


	//?? flush?
	data[0] = 'd';
	if (usb_out(1, data)!=1) {
		fprintf(stderr, "ERROR sending flush(?)\n");
		data_prov->done(data_prov->read_data);
		return -1;
	}

	if (usb_in(4, data)!=4) {
		fprintf(stderr, "ERROR receiving 'd' report\n");
		data_prov->done(data_prov->read_data);
		return -1;
	}

	if (verbose) {
		printf("flush: "); hexdump(data, 4);
	}

	data_prov->done(data_prov->read_data);


	printf("\n");

	return 0;
 }



static int read_file_function(void *d, unsigned char *data, int size)
{
	FILE *f = (FILE *)d;
	return fread(data, 1, size, f);
}

static void done_file_function(void *d)
{
	FILE *f = (FILE *)d;
	fclose(f);
}

/*
 * Offset -> offset in flash
 * Erase Size -> Size to erase
 * filename -> file to write
 */

int sendfile_to_device(unsigned int offset,
		       int erasesize,
		       int partition_id,
		       const char *filename)
{
	data_provider dp;
	long fsize;
	FILE * f = fopen(filename, "r");
	if (!f) {
		printf("error reading file\n");
		return  -1;
	}
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	dp.offset = offset;
	dp.erase_size = erasesize;
	dp.partition_id = partition_id;
	dp.file_size = fsize;
	dp.read_data = f;
	dp.read = read_file_function;
	dp.done = done_file_function;
	return  send_to_device(&dp);
}

typedef struct {
	FILE *f;
	int size;
	int read_so_far;
} nb0_read_data_noptt;


static int nb0_read_file_function(void *d, unsigned char *data, int size)
{
	nb0_read_data_noptt *np = (nb0_read_data_noptt *)d;

	int max_read = np->size - np->read_so_far;

	if ( size > max_read) {
		size = max_read;
	}

	if (size<=0) {
		return 0;
	}

	int n =  fread(data, 1, size, np->f);

	np->read_so_far += n;

	return n;

}

static void nb0_done_file_function(void *d)
{
	nb0_read_data_noptt *np = (nb0_read_data_noptt *)d;
	fclose(np->f);
}


typedef struct {
	FILE *f;
	int ptt_size;
	int data_size;
	long size;
	long data_offset;
	int read_so_far;
} nb0_read_data_ptt;



static int read_file_function_with_ptt(void *d, unsigned char *data, int size)
{
	nb0_read_data_ptt *np = (nb0_read_data_ptt *)d;



	if (np->read_so_far >= np->ptt_size) {
		//read the data part
		int max = np->size - np->read_so_far;
		if (size> max) {
			size = max;
		}

		if (size<=0) {
			return 0;
		}

		int n = fread(data, 1, size, np->f);

		np->read_so_far += n;

		return n;
	} else {
		//read header part
		if ((np->read_so_far + size) > np->ptt_size) {
			//read from two places
			int leftover_in_ptt = np->ptt_size - np->read_so_far;
			int n1 =  fread(data, 1, leftover_in_ptt, np->f);
			fseek(np->f, np->data_offset, SEEK_SET);

			size -= n1;

			if (size > np->data_size) {
				size = np->data_size;
			}



			int n2 =  fread(data + leftover_in_ptt, 1, size, np->f);

			np->read_so_far += n1 + n2;

			return n1 + n2;

		} else {
			//read from ptt_header only
			int n =  fread(data, 1, size, np->f);
			np->read_so_far += n;
			if (np->read_so_far == np->ptt_size) {
				fseek(np->f, np->data_offset, SEEK_SET);
			}
			return n;
		}
	}
}

static void done_file_function_with_ptt(void *d)
{
	nb0_read_data_ptt *np = (nb0_read_data_ptt *)d;
	fclose(np->f);
}

/*
 * Offset -> offset in flash
 * Erase Size -> Size to erase
 * filename -> file to write
 */

static int sendfile_nb0_package_to_device(const char *filename,
				   const char *package_name)
{
	data_provider dp;
	FILE *f;
	int i;
	nb0_header_item *header = 0;
	nb0_header_item *ptt_header = 0;
	char tmpfilename[FILENAME_MAX];
	package_write_info *write_info = 0;



	strcpy(tmpfilename, package_name);
	strcat(tmpfilename, ".ptt_header");

	for (i=0; i < nb0_count; i++) {
		if (strcmp(nb0_headers[i].name, package_name)==0) {
			header = &nb0_headers[i];
		}
		if (strcmp(nb0_headers[i].name, tmpfilename)==0) {
			ptt_header = &nb0_headers[i];
		}

	}

	if (!header) {
		printf("package '%s' not found", package_name);
		return -1;
	}

	for (i=0; i < package_info_count; i++) {
		printf("package : %s\n", package_info[i].name);
		if (strcmp(package_info[i].name, package_name)==0) {
			write_info = &package_info[i];
			break;
		}
	}

	if (!write_info) {
		printf("package info for '%s' not found", package_name);
		return -1;
	}


	f = fopen(filename, "r");
	if (!f) {
		printf("error reading file\n");
		return  -1;
	}

	dp.offset = write_info->start_address;
	dp.erase_size = write_info->max_size;
	dp.partition_id = write_info->partition_id;
	dp.file_size = header->size;

	if (!ptt_header) {

		printf("Sending file: %s (without ptt_header)\n", package_name);

		nb0_read_data_noptt np;

		np.f = f;
		np.size = header->size;
		np.read_so_far = 0;

		fseek(f, header->nb0_file_offset, SEEK_SET);

		dp.read_data = &np;
		dp.read = nb0_read_file_function;
		dp.done = nb0_done_file_function;
		int ret =  send_to_device(&dp);
		return ret;
	} else {
		printf("Sending file: %s (with ptt_header)\n", package_name);

		nb0_read_data_ptt np;

		np.f = f;
		np.size = header->size + ptt_header->size;
		np.read_so_far = 0;
		np.ptt_size = ptt_header->size;
		np.data_size = header->size;
		np.data_offset = header->nb0_file_offset;

		dp.read_data = &np;
		dp.read = read_file_function_with_ptt;
		dp.done = done_file_function_with_ptt;

		int ret = send_to_device(&dp);
		return ret;
	}

}


int write_nb0_file(int argc, char *argv[])
{
	int i, r;
	int write_all = 0;
	char *nb0 = 0;
	int package_count = 0;
	char **packagenames = 0;
        unsigned char data[128];
	char *err;

	if (argc<3)
		return print_usage();


	/*yes, i know about getopt, i am doing manual parsing to make
	  this app easier to port (less dependency)*/
	i = 2; 
	while (i<argc) {

		if (strcmp(argv[i], "-d")==0) {
			verbose = 1;
			i++;
			continue;
		}
		if (strcmp(argv[i], "-p")==0) {
			if (i+1 >= argc)
				return print_usage();
			product_id = strtol(argv[i+1], &err, 16);
			if (*err) {
				fprintf(stderr, "invalid PID\n");
				return -1;
			}
			i+=2;
			continue;
		}

		if (strcmp(argv[i], "-v")==0) {
			if (i+1 >= argc)
				return print_usage();
			vendor_id = strtol(argv[i+1], &err, 16);
			if (*err) {
				fprintf(stderr, "invalid VID\n");
				return -1;
			}
			i+=2;
			continue;
		}

		if (strcmp(argv[i], "-i")==0) {
			if (i+1 >= argc)
				return print_usage();
			in_endpoint = strtol(argv[i+1], &err, 0);
			if (*err) {
				fprintf(stderr, "invalid in endpoint\n");
				return -1;
			}
			i+=2;
			continue;
		}

		if (strcmp(argv[i], "-o")==0) {
			if (i+1 >= argc)
				return print_usage();
			out_endpoint = strtol(argv[i+1], &err, 0);
			if (*err) {
				fprintf(stderr, "invalid out endpoint\n");
				return -1;
			}
			i+=2;
			continue;
		}

		if (nb0==0) {
			nb0 = argv[i];
			i++;
		} else {
			packagenames = realloc(packagenames, 
					       sizeof(char *)*(package_count+1));
			packagenames[package_count] = argv[i];
			package_count++;
			i++;
		}

	}


	write_all = (package_count==0);

	if (extract_nb0(nb0, 0)!=0) {
		fprintf(stderr, "ERROR: failed reading nb0 file\n");
		return -1;
	}

	int found = 0;

	for (i=0; i < nb0_count; i++) {
		char *name = nb0_headers[i].name;
		int namelen = strlen(name);

		if (namelen<=4)
			continue;
		if (strcmp(".mlf", name + namelen-4)!=0)
			continue;

		printf("Found mlf: %s\n", name);
		FILE *f = fopen(nb0, "r");
		if (!f) {
			fprintf(stderr, "ERROR: failed opening : %s\n", nb0);
			free_nb0();
			return -1;
		}

		fseek(f, nb0_headers[i].nb0_file_offset, SEEK_SET);
		char *data = (char*)malloc(nb0_headers[i].size+1);

		fread(data, 1, nb0_headers[i].size, f);

		fclose(f);
		data[nb0_headers[i].size] = '\0';
		r = parse_mlf(data);
		free(data);

		if (r!=0) {
			fprintf(stderr, "ERROR parsing MLF\n");
			free_nb0();
			return -1;
		}

		found = 1;
		break;

	}

	if (!found) {
		fprintf(stderr, "ERROR: NB0 doesn't contain any MLF\n");
		free_nb0();
		free_mlf();
		return -1;
	}

        r = libusb_init(NULL);
        if (r < 0) {
		free_nb0();
		free_mlf();
		return -1;
	}

	if (verbose) {
		libusb_device **devs;

		r = libusb_get_device_list(NULL, &devs);

		if (r < 0) {
			free_nb0();
			free_mlf();
			libusb_exit(NULL);
			return -1;
		}
	
		print_devs(devs);

		libusb_free_device_list(devs, 1);

	}

       if (find_device()!=0) {
	       fprintf(stderr, "ERROR: can not find device (VID:PID = %04x:%04x)\n", vendor_id, product_id);
	       free_nb0();
	       free_mlf();
	       libusb_exit(NULL);
	       return -1;
        }

        r = libusb_claim_interface(devh, 0);
        if (r < 0) {
                fprintf(stderr, "usb_claim_interface error %d\n", r);
		free_nb0();
		free_mlf();
		libusb_exit(NULL);		
		return -1;
        }


        data[0] = 'V';  //version?

        if (usb_out(1, data)!=1) {
                fprintf(stderr, "ERROR sending 'V' command\n");
                goto free_claimed_usb;
        }

        if (usb_in(1, data)!=1) {
                fprintf(stderr, "ERROR receiving 'V' response\n");
                goto free_claimed_usb;
        } else {
		if (verbose) {
			printf("V response %d\n", data[0]); //4
		}
                if (data[0]!=4) {
                        fprintf(stderr, "V is not 4, aborting\n");
                        goto free_claimed_usb;
                }
        }

        data[0] = 'O'; //version string
        if (usb_out(1, data)!=1) {
                fprintf(stderr, "ERROR sending 'O' command\n");
                goto free_claimed_usb;
        }

        if (usb_in(4, data)!=4) {
                fprintf(stderr, "ERROR receiving 'O' response\n");
                goto free_claimed_usb;
        } else {
		if (verbose) {
			data[4] = '\0';
			printf("Response for O = %s\n", data);
		}
        }

        data[0] = 'I'; //info??

        if (usb_out(1, data)!=1) {
                fprintf(stderr, "ERROR sending 'I' command\n");
                goto free_claimed_usb;
        }

        if (usb_in(24, data)!=24) {
                fprintf(stderr, "ERROR receving 'I' response\n");
                goto free_claimed_usb;
        } else {
		if (verbose) {
			printf("I response: ");
			hexdump(data, 24);
		}
        }

	if (write_all) {
		for (i=0; i < package_info_count; i++) {
			sendfile_nb0_package_to_device(nb0, package_info[i].name);
		}
	} else {
		for (i=0; i < package_count; i++) {
			sendfile_nb0_package_to_device(nb0, packagenames[i]);
		}
	}


        data[0] = 'C'; //restart
        usb_out(1, data);


 free_claimed_usb:
	libusb_release_interface(devh, 0);
        libusb_exit(NULL);	
	free_nb0();
	free_mlf();

	return 0;
}
