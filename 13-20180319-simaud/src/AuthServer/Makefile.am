RABBITMQ_LDFLAGS = -L/usr/local/lib/x86_64-linux-gnu
RABBITMQ_LIBS = -lrabbitmq

# --------------------------------------------------------

NULL =
# --------------------------------------------------------

noinst_LTLIBRARIES=libsimau.la

libsimau_la_SOURCES =				\
				simaud.h			\
				simaudauthority.h		simaudauthority.c	\
				simaudauthserver.h		simaudauthserver.c	\
				simaudnetlink.h			simaudnetlink.c 	\
				simaudrulefile.h		simaudrulefile.c	\
				simaudruleinterpret.h	simaudruleinterpret.c\
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
