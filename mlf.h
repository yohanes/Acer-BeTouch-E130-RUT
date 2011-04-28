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
#ifndef MLF_H
#define MLF_H

typedef struct {
	int package_number;
	int index;
	char name[49];
	int start_address;
	int max_size;
	int partition_id;
} package_write_info;

extern package_write_info *package_info;
extern int package_info_count;

/**
 * Returns 0 on success, nonzero otherwise
 *
 * Will fill in package_info and package_info_count on success
 */
int parse_mlf(char *data);

void free_mlf();

#endif
