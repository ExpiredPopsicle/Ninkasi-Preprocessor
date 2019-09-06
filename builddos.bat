del *.obj
wcc -mh main.c
wcc -mh ppcommon.c
wcc -mh ppdirect.c
wcc -mh ppmacro.c
wcc -mh ppstate.c
wcc -mh ppstring.c
wcc -mh pptoken.c
wcc -mh ppmem.c
wcc -mh pppath.c
wcc -mh pptest.c
wcc -mh ppexpr.c
wcc -mh pperror.c
wcc -mh ppx.c
wcl -mh *.obj

echo "DONE"
