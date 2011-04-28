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
#ifndef NB0_H
#define NB0_H

/**
 * returns 0 on success
 * will fill in global variable nb0_count and nb0_headers
 */
int extract_nb0(const char *filename, const char *extract_to);

int create_nb0(const char *filename, const char *from_dir);

typedef struct {
	long offset;
	long size;
	char name[49];
	/*additional info for direct reading*/
	long nb0_file_offset;
} nb0_header_item;

void free_nb0();

extern int nb0_count;
extern nb0_header_item *nb0_headers;

#endif
