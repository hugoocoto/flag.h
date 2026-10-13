VERSION := $(shell sed -n 's/^#define FLAG_H_VERSION "\(.*\)"/\1/p' flag.h)

all: example

example: example.c flag.h
	gcc ./example.c -std=c99 -o example -ggdb -Wall -Wextra -pedantic

clean:
	rm -f example flag.h-*.zip

package: flag.h
	rm -f flag.h-*.zip
	zip flag.h-$(VERSION).zip $^
