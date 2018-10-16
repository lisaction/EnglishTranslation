#include <sys/inotify.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "formatter.h"
#include "src/AuthServer/simaudruleinterpret.h"
#include "src/AuthServer/simauduserif.h"


int main(int argc, char* argv[]){

	int fd, wd, poll_num;
	nfds_t nfds;
	struct pollfd fds[1];

	/* opt */
	if (argc > 1){
		if (strcmp(argv[1],"--no-debug")==0){
			/* printf("test here.\n"); */
			freopen("/dev/null","w", stdout);
			freopen("/dev/null","w", stdin);
			freopen("/dev/null","w", stderr);
		}
	}

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		die_on_error("inotify_init1");
	}

	wd = inotify_add_watch(fd, get_rule_path(), IN_CLOSE_WRITE);
	if (wd == -1){
		die_on_error("Can't watch rule file");
	}

	nfds = fd;
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (1){
		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1){
			if (errno == EINTR)
				continue;
			die_on_error("poll");
		}

		if (poll_num > 0){
			if (fds[0].revents & POLLIN){
				//handle_file_events(fd, wd);
				formatter_write_2newfile();
			}
			else {
				fprintf(stderr, "Warning: Unknown file descriptor.\n");
			}
		}

	}

}
