#########################################################################
 # This file is part of Greedtella.
 #
 # Greedtella is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # Greedtella is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with Greedtella; if not, write to the Free Software
 # Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 #
 # Copyright 2003 William Bornor
 #
 # $Id: Makefile,v 1.1.1.1 2005/01/31 21:50:19 wbornor Exp $
 ########################################################################/

CC=gcc
CFLAGS=-g -Wall
HDRS=greedtella.h
OPTMZ=-O2
TOBJS=greedtellalib.o tests.o 

archive: 
	tar -czvf greedtellasrc.tar.gz *.c *.h README COPYING Makefile 

tests: $(TOBJS)
	$(CC) $(CFLAGS) -o tests $(TOBJS)
	./tests

greedtellalib.o: greedtellalib.c $(HDRS)
	$(CC) $(CFLAGS) -c -o greedtellalib.o greedtellalib.c

tests.o: tests.c $(HDRS)
	$(CC) $(CFLAGS) -c -o tests.o tests.c
clean:
	rm -f tests $(TOBJS)
