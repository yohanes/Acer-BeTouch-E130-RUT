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

#include <stdlib.h>
#include <string.h>
#include "util.h"

void trim(char *str)
{
	int len;
	char *s = strdup(str);
	//trim from back
	int i = strlen(s)-1;
	while (i>=0 && (s[i]=='\r' || s[i]=='\n' || s[i]==' ')) {
		s[i] = '\0';
		i--;
	}
	len = strlen(s);
	i = 0;
	while (i<len) {
		if (s[i]!=' ' && s[i]!='\r' && s[i]!='\n') 
			break;
		i++;
	}
	strcpy(str, s + i);
	free(s);
}
