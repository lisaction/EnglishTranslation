RABBITMQ_LDFLAGS = -L/usr/local/lib/x86_64-linux-gnu
RABBITMQ_LIBS = -lrabbitmq

# --------------------------------------------------------

NULL =
# --------------------------------------------------------

noinst_LTLIBRARIES=libmqclient.la

libmqclient_la_SOURCES =				\
				messagequeue.c		messagequeue.h 	\
				mqcommand.c			mqcommand.h		\
				mqutils.c			mqutils.h		\
					$(NULL)

libmqclient_la_LDFLAGS = 				\
				$(RABBITMQ_LDFLAGS)		\
				$(SIMAUD_LDFLAGS)		\
				$(NULL)

libmqclient_la_LIBADD = 			\
				$(RABBITMQ_LIBS)	\
				$(NULL)			


# --------------------------------------------------------
bin_PROGRAMS = mqclient
mqclient_SOURCES = MqClient.c

mqclient_LDADD = libmqclient.la		\
				$(top_builddir)/src/AuthServer/libsimau.la	\
				$(NULL)

# --------------------------------------------------------

clean-local :
	rm -f *~
