# CS 111 Lab 1 Makefile

CC = gcc
CFLAGS = -g -Wall -Wextra #-Werror
LAB = 1
DISTDIR = lab1-$(USER)
CHECK_DIST = ./check-dist

all: timetrash

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

TIMETRASH_SOURCES = \
  alloc.c \
  execute-command.c \
  main.c \
  read-command.c \
  print-command.c
TIMETRASH_OBJECTS = $(subst .c,.o,$(TIMETRASH_SOURCES))

# my insertions
TEST_SOURCES = \
  alloc.c \
  execute-command.c \
  testmain.c \
  read-command.c \
  print-command.c
TEST_OBJECTS = $(subst .c,.o,$(TEST_SOURCES))
# end insertion

DIST_SOURCES = \
  $(TIMETRASH_SOURCES) alloc.h command.h command-internals.h Makefile \
  $(TESTS) check-dist README

timetrash: $(TIMETRASH_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(TIMETRASH_OBJECTS)

# my insertion
testall: $(TEST_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJECTS)
# end insertion

alloc.o: alloc.h
execute-command.o main.o print-command.o read-command.o: command.h
execute-command.o print-command.o read-command.o: command-internals.h

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) check-dist
	rm -fr $(DISTDIR)
	tar -czf $@.tmp --mode=u+w --transform='s,^,$(DISTDIR)/,' \
	  $(DIST_SOURCES)
	$(CHECK_DIST) $(DISTDIR)
	mv $@.tmp $@

Skeleton: $(DIST_SOURCES)
	$(MAKE) CHECK_DIST=: USER=$@ lab1-$@.tar.gz

check: $(TEST_BASES)

$(TEST_BASES): timetrash
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp timetrash $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean Skeleton
