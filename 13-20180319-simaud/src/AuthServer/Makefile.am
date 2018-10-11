# --------------------------------------------------------

NULL =
# --------------------------------------------------------

noinst_LTLIBRARIES=libsimau.la

libsimau_la_SOURCES =				\
				simauduserif.h			simauduserif.c	\
				simaudkernelif.h		simaudkernelif.c	\
				simaudrulefile.h		simaudrulefile.c	\
				simaudruleinterpret.h	simaudruleinterpret.c\
				simaudrequest.h			simaudrequest.c		\
					$(NULL)

# --------------------------------------------------------
bin_PROGRAMS = simaud
simaud_SOURCES = simaud.c

simaud_LDADD = libsimau.la

# --------------------------------------------------------

clean-local :
	rm -f *~
