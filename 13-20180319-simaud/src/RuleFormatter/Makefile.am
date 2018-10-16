NULL =
# --------------------------------------------------------

noinst_LTLIBRARIES=libformatter.la

libformatter_la_SOURCES =				\
				formatter.c		formatter.h 	\
					$(NULL)

# --------------------------------------------------------
bin_PROGRAMS = formatterd
formatterd_SOURCES = formatterd.c

formatterd_LDADD = libformatter.la		\
				$(top_builddir)/src/AuthServer/libsimau.la	\
				$(NULL)

# --------------------------------------------------------

clean-local :
	rm -f *~
