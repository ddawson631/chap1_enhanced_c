#
# Makefile for C version of enhanced chapter 1 interpeter. 
# The chapter 1 interpreter from Samuel Kamin's Programming Languages texbook
# was enhanced to use a more Pascal-like syntax instead of Lisp-style syntax.
#
SHELL = /bin/bash

chap1: chap1.o
	gcc -o chap1 chap1.o

chap1.o: chap1.c
	gcc -std=c99 -c chap1.c

debug:
	gcc -std=c99 -g -o chap1 chap1.c
#
# The testsize target below builds with TESTSIZE macro defined.
# This causes the MAXNAMES and MAXINPUT macros to be assigned smaller
# sizes so that unit test cases that exceed these values can be performed.
#
testsize:
	gcc -std=c99 -DTESTSIZE -o chap1 chap1.c

clean:
	rm -f chap1.o

