# $Id$
CFLAGS=-ggdb
SRCS=main.c
OBJS=$(SRCS:.c=.o)

cflow: $(OBJS)
	$(CC) $(CFLAGS) -o cflow $(OBJS)

clean:; rm $(OBJS) 

TAGS:; etags $(SRCS)


dep:
	$(CPP) -MM $(SRCS) > .depend

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
