VERSION := $(shell git describe --tags --match 'v*' --always --dirty 2>/dev/null)
ifeq ($(VERSION),)
	VERSION := v$(shell cat VERSION 2>/dev/null || echo unknown)
endif

all: example

example: example.c flag.h
	gcc ./example.c -std=c99 -o example -ggdb -Wall -Wextra -pedantic

clean:
	rm -f example flag.h-*.zip

package: flag.h
	rm -f flag.h-*.zip
	zip flag.h-$(VERSION).zip $^
