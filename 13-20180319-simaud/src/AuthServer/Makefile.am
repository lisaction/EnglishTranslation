RABBITMQ_LDFLAGS = -L/usr/local/lib/x86_64-linux-gnu
RABBITMQ_LIBS = -lrabbitmq

# --------------------------------------------------------

NULL =
# --------------------------------------------------------

noinst_LTLIBRARIES=libsimau.la

libsimau_la_SOURCES =				\
				simauduserif.h			simauduserif.c	\
				simaudrulefile.h		simaudrulefile.c	\
				simaudruleinterpret.h	simaudruleinterpret.c\
				simaudrequest.h			simaudrequest.c		\
					$(NULL)

libsimau_la_LDFLAGS = 				\
				$(RABBITMQ_LDFLAGS)	\
				$(NULL)

libsimau_la_LIBADD = 				\
				$(RABBITMQ_LIBS)	\
				$(NULL)				

# --------------------------------------------------------
bin_PROGRAMS = simaud
simaud_SOURCES = simaud.c

simaud_LDADD = libsimau.la

# --------------------------------------------------------

clean-local :
	rm -f *~
