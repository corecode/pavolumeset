PROG=pavolumeset

CFLAGS+=	-std=c99 -Wall -g
LDLIBS+=	-lpulse

all: ${PROG}

clean:
	-rm -f ${PROG}
