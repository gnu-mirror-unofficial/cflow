# $Id$
CFLAGS=-ggdb
SRCS=main.c parser.c lex.yy.c collect.c output.c symbol.c
OBJS=$(SRCS:.c=.o)

cflow: $(OBJS)
	$(CC) $(CFLAGS) -o cflow $(OBJS) -ll

lex.yy.c: c.l cflow.h
	lex -bvpp c.l  2> cflow.lex.report


clean:; rm $(OBJS) 

TAGS:; etags $(SRCS)

ci:; ci -l $(SRCS)

dep:
	$(CPP) -MM $(SRCS) > .depend

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
