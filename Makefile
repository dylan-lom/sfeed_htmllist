sfeed_htmllist: sfeed_htmllist.c util.c util.h
	cc -o sfeed_htmllist -Wall -Wextra -pedantic -ggdb util.c sfeed_htmllist.c -lbsd

clean:
	rm sfeed_htmllist
