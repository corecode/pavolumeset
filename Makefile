PROG=pavolumeset

CFLAGS+=	-std=c99 -Wall -g -O
LDLIBS+=	-lpulse

all: ${PROG}

clean:
	-rm -f ${PROG}
