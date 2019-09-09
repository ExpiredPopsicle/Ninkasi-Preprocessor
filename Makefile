all : a_nonafl b

a_nonafl : main.c ppmacro.c ppstate.c ppcommon.c ppstring.c pptoken.c ppdirect.c ppmem.c pppath.c pptest.c ppexpr.c pperror.c ppx.c
	$(CC) -g -Wall $^ -o $@

b : simple.c ppmacro.c ppstate.c ppcommon.c ppstring.c \
	pptoken.c ppdirect.c ppmem.c pppath.c pptest.c ppexpr.c \
	pperror.c ppx.c

	$(CC) -g -Wall $^ -o $@
