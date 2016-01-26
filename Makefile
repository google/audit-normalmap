CFLAGS ?= -std=c99 -Wall -Wextra -O3
LDLIBS ?= -lm

all: audit-normalmap

audit-normalmap: audit-normalmap.o
audit-normalmap.o: audit-normalmap.c \
	third_party/stb/stb_image.h third_party/stb/stb_image_write.h

clean:
	$(RM) audit-normalmap audit-normalmap.o
