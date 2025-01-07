CFLAGS+=	-Wall -Wno-parentheses -Wno-missing-braces -O3 -g -DNDEBUG
RM=		rm -f
UTF16FIXOBJ=	utf16fix_generic.o \
		utf16fix_impls.o \
		utf16fix_ref.o \
		utf16fix_sse.o

all: benchmark test

test: test.o $(UTF16FIXOBJ)
	$(CC) $(LDFLAGS) -o $@ test.o $(UTF16FIXOBJ)

benchmark: benchmark.o benchframework.o $(UTF16FIXOBJ)
	$(CC) $(LDFLAGS) -o $@ benchmark.o benchframework.o $(UTF16FIXOBJ)

clean:
	$(RM) *.o *.s benchmark test

benchframework.o: benchmark.h
benchmark.o: benchmark.h utf16fix.h
test.o: utf16fix.h
utf16fix_generic.o: utf16fix.h
utf16fix_impls.o: utf16fix.h
utf16fix_ref.o: utf16fix.h
utf16fix_sse.o: utf16fix.h

.PHONY:
	clean
