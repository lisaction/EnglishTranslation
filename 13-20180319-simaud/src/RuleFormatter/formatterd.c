#include <sys/inotify.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "formatter.h"
#include "src/AuthServer/simaudrulefile.h"
#include "src/AuthServer/simauduserif.h"


// watch the dir
static int formatter_create_inotify_fd(){
	int fd, wd;

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		die_on_error("inotify_init1");
	}

	wd = inotify_add_watch(fd, get_rule_path(), IN_CLOSE_WRITE);
	if (wd == -1){
		die_on_error("Can't watch rule file");
	}

	return fd;

}

int main(int argc, char* argv[]){

	int fd, poll_num;
	nfds_t nfds;
	struct pollfd fds[1];
	char buf[4096];

	/* opt */
	if (argc > 1){
		if (strcmp(argv[1],"--no-debug")==0){
			/* printf("test here.\n"); */
			freopen("/dev/null","w", stdout);
			freopen("/dev/null","w", stdin);
			freopen("/dev/null","w", stderr);
		}
	}

	nfds = 1; //number

	while (1){
		fd = formatter_create_inotify_fd();
		fds[0].fd = fd;
		fds[0].events = POLLIN;

		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1){
			if (errno == EINTR)
				continue;
			die_on_error("poll");
		}

		if (poll_num > 0){
			if (fds[0].revents & POLLIN){
				//read (fd, buf, sizeof(buf)); //flush
				//handle_file_events(fd, wd);
				formatter_write_2newfile();
			}
			else {
				fprintf(stderr, "Warning: Unknown file descriptor.\n");
			}
		}
		close(fd);
	}

}
