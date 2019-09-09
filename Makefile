all : a b

a : example/main.c src/ppmacro.c src/ppstate.c src/ppcommon.c src/ppstring.c \
	src/pptoken.c src/ppdirect.c src/ppmem.c src/pppath.c src/pptest.c src/ppexpr.c \
	src/pperror.c src/ppx.c

	$(CC) -Isrc -g -Wall $^ -o $@

b : example/simple.c src/ppmacro.c src/ppstate.c src/ppcommon.c src/ppstring.c \
	src/pptoken.c src/ppdirect.c src/ppmem.c src/pppath.c src/pptest.c src/ppexpr.c \
	src/pperror.c src/ppx.c

	$(CC) -Isrc -g -Wall $^ -o $@
