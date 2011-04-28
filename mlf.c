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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mlf.h"
#include "util.h"


package_write_info *package_info = 0;
int package_info_count = 0;

typedef struct {
	char *section;
	char *name;
	char *value;
} config_value;


static char *get_config_value(config_value *configs, int size, 
	       const char *section, const char *name)
{
	int i;
	for (i = 0; i < size; i++) {
		if (strcmp(configs[i].section, section)==0) {
			if (strcmp(configs[i].name, name)==0) {
				return configs[i].value;
			}
		}
	}
	return 0;
}


/**
 * Todo: check version (ptt version?)
 *
 */
int parse_mlf(char *data)
{
	int error = 0;
	int i;
	char line[256];
	char current_section[256];
	char name[128];
	char value[128];

	
	config_value *configs = 0;
	int config_ctr =0;

	int section_ctr = 0;
	char **section_names = 0;

	int file_len = strlen(data);
	int file_pos = 0;

	while (file_pos<file_len && data[file_pos]) {

		int j = 0;

		line[0] = '\0';
		while (file_pos<file_len) {
			line[j] = data[file_pos];
			if (line[j]=='\r' || line[j]=='\n') {
				line[j]='\0';

				while (file_pos < file_len &&
				       (data[file_pos]=='\r' ||
					data[file_pos]=='\n')) {
					file_pos++;
				}
				break;
			}
			file_pos++;
			j++;

		}

		if (j==0) 
			break;

		if (file_pos>=file_len) {
			line[j+1]='\0';
		}


		trim(line);

		if (strlen(line)==0) 
			continue;

		if (line[0]=='[') {

			assert(strlen(line)>2);
			assert(line[strlen(line)-1]==']');

			strcpy(current_section, line + 1);
			current_section[strlen(current_section)-1] = '\0';
			//printf("current_section: %s\n", current_section);
			section_names = (char **)realloc(section_names, sizeof(char *)*(section_ctr+1));
			section_names[section_ctr] = strdup(current_section);
			section_ctr++;
			continue;
		}


		char * _name = strtok(line, "=");
		strcpy(name, _name);
		trim(name);
		char * _value = strtok(0, "=");
		value[0] ='\0';
		if (_value) {
			strcpy(value, _value);
			trim(value);

			
			configs = (config_value*)realloc(configs, sizeof(config_value)*
							 (config_ctr+1));

			configs[config_ctr].section = strdup(current_section);
			configs[config_ctr].name = strdup(name);
			configs[config_ctr].value = strdup(value);
			
			config_ctr++;
		}
	}

	

	for (i = 0; i < section_ctr; i++) {

	        const char * section_key = "Package Info ";


		if (strncmp(section_names[i], section_key, 
			    strlen(section_key))==0) {
			char *number = section_names[i] + strlen(section_key);

			char *err;

			int package_number = strtol(number, &err, 10);

			if (*err!='\0') continue;

			//printf("%s number: %d '%c'\n", number, package_number, *err);


			char *image_file = get_config_value(configs, config_ctr, 
							    section_names[i], "IMAGE FILE");
			if (!image_file) {
				error = 1;
				fprintf(stderr, "can not find 'IMAGE FILE' for section '%s' in mlf\n", section_names[i]);
				break;
			}

			if (strlen(image_file)>48) {
				error = 1;
				fprintf(stderr, "image file name '%s' is too long (%zd characters, max should be 48)\n", 
				       image_file, strlen(image_file));
				break;
			}

			char *start_address = get_config_value(configs, config_ctr, 
							    section_names[i], "START ADDRESS");

			if (!start_address) {
				error = 1;
				fprintf(stderr, "can not find 'START ADDRESS' for section '%s' in mlf\n", section_names[i]);
				break;
			}

			//HEX for start address
			int _start_address = strtol(start_address, &err, 16);

			if (*err!='\0') {
				error = 1;
				fprintf(stderr, "Invalid START ADDRESS value '%s' in mlf\n", start_address);
				break;				
			}
			

			char *index = get_config_value(configs, config_ctr, 
							    section_names[i], "INDEX");

			if (!index) {
				error = 1;
				fprintf(stderr, "can not find 'INDEX' for section '%s' in mlf\n", section_names[i]);
				break;
			}

			int _index = strtol(index, &err, 10);

			if (*err!='\0') {
				error = 1;
				fprintf(stderr, "Invalid INDEX value '%s' in mlf\n", index);
				break;				
			}

			char *partition_id = get_config_value(configs, config_ctr, 
							    section_names[i], "PARTITION_ID");

			if (!partition_id) {
				error = 1;
				fprintf(stderr, "can not find 'PARTITION_ID' for section '%s' in mlf\n", section_names[i]);
				break;
			}			

			int _partition_id = strtol(partition_id, &err, 10);

			if (*err!='\0') {
				error = 1;
				fprintf(stderr, "Invalid PARTITION_ID value '%s' in mlf\n", partition_id);
				break;				
			}


			char *max_size = get_config_value(configs, config_ctr, 
							    section_names[i], "MAX_SIZE");

			if (!max_size) {
				error = 1;
				fprintf(stderr, "can not find 'MAX_SIZE' for section '%s' in mlf\n", section_names[i]);
				break;
			}	

			int _max_size = strtol(max_size, &err, 10);

			if (*err!='\0') {
				error = 1;
				fprintf(stderr, "Invalid MAX_SIZE value '%s' in mlf\n", max_size);
				break;				
			}
		
			

			package_info = (package_write_info *)realloc(package_info, 
								     sizeof(package_write_info)*
								     (package_info_count+1));


			package_info[package_info_count].package_number = package_number;
			package_info[package_info_count].index = _index;
			strcpy(package_info[package_info_count].name, image_file);			
			package_info[package_info_count].start_address = _start_address;		       
			package_info[package_info_count].partition_id = _partition_id;
			package_info[package_info_count].max_size = _max_size;

			package_info_count++;


		}

	}

	printf("package count = %d\n", package_info_count);

	for (i = 0; i < section_ctr; i++) {
		free(section_names[i]);
	}
	free(section_names);

	for (i=0; i < config_ctr; i++) {
		free(configs[i].section);
		free(configs[i].name);
		free(configs[i].value);
	}	

	free(configs);

	if (error) {
		free(package_info);		
		package_info_count = 0;
					
	}

	return error;
}



void free_mlf()
{
	free(package_info);
	package_info_count = 0;		
	package_info = 0;
}

#ifdef TEST_MLF

static char *read_file(const char *filename)
{
	long size;
	char *data;
	FILE *f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (char*)malloc(size+1);
	fread(data, 1, size, f);
	data[size] = '\0';
	fclose(f);
	return data;
}

static int test_main_mlf(int argc, char *argv[])
{
	char *data = read_file("Acer_E130_3.002.70_AAP_GEN2TH_P2_EU.mlf");
	printf("Data: %s", data);
	parse_mlf(data);
	free(data);
	return 0;
}

int main(void)
{
	test_main_mlf(0, 0);
	return 0;
}

#endif
