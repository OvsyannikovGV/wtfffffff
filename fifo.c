#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#define my_assert_(condition)                                   \
        if (condition)                                          \
        {                                                       \
            fprintf(stderr, "error at line %d\n", __LINE__);    \
            return -1;                                          \
        }                                           

#define PID_FIFO  "pid_fifo"
#define FIFO_NAME_SIZE 100
#define BUFFSIZE 256
#define TIME_LIMIT 1000
#define SMALL_TIME 100
const mode_t CHMOD = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

int receiver ();
int sender (const char* file_name);
int waiting_sender (const int fd);

int main (int argc, char* argv[])
{
	if (argc == 1)
	{
		receiver ();
	}

	else if (argc == 2)
	{
		sender (argv[1]);
	}

	else
	{
		perror("wrong num of args\n");
		return 1;
	}
	return 0;
}

int receiver ()
{
	int check_mkfifo_pid = mkfifo (PID_FIFO, CHMOD);
	my_assert_(check_mkfifo_pid == -1 && errno != EEXIST);
	
	pid_t pid = getpid ();
	
	int pid_fd = open (PID_FIFO, O_WRONLY);
	my_assert_(pid_fd == -1);
		
	char fifo_name[FIFO_NAME_SIZE] = {0};

	my_assert_(sprintf (fifo_name, "tempfifo%d", pid) < 0);

	int check_mkfifo = mkfifo (fifo_name, CHMOD);
	my_assert_(check_mkfifo == -1 && errno != EEXIST);
	
	int fifo_fd = open (fifo_name, O_RDONLY | O_NONBLOCK);
	my_assert_(fifo_fd == -1);

	int write_pid = write (pid_fd, &pid, sizeof (pid_t));
	my_assert_(write_pid == -1);
	my_assert_(close (pid_fd) == -1);

	my_assert_(waiting_sender (fifo_fd) > 0);
	
	fcntl(fifo_fd, F_SETFL, O_RDONLY);

	char buffer[BUFFSIZE] = {0};

	int count_symbols = -1;

	while (count_symbols != 0)
	{
		count_symbols = read (fifo_fd, buffer, BUFFSIZE);
		write (STDOUT_FILENO, buffer, count_symbols);
	}

	my_assert_(close (fifo_fd) == -1);
    my_assert_(unlink (fifo_name));	
	
	return 0;
}

int waiting_sender (const int fd)
{
	struct pollfd pfd = {
        .fd = fd, 
        .events = POLLIN
    };

	int res = poll (&pfd, 1, TIME_LIMIT);
	if(res == 0 || res == -1 || pfd.revents != POLLIN) {
		errno = ETIME;
		return -2;
	}
	
	return 0;
}

int sender (const char* file_name)
{
	int check_mkfifo_pid = mkfifo (PID_FIFO, CHMOD);
	my_assert_(check_mkfifo_pid == -1 && errno != EEXIST);
	
	int pid_fd = open (PID_FIFO, O_RDWR);
	my_assert_(pid_fd == -1);
	
	pid_t pid = -1;
	
	int check_read = read (pid_fd, &pid, sizeof (pid_t));
	my_assert_(check_read == -1);
	
	my_assert_(close (pid_fd) == -1);

	char fifo_name[FIFO_NAME_SIZE] = {0};

	my_assert_(sprintf (fifo_name, "tempfifo%d", pid) == -1);
	
	int fifo_fd = open (fifo_name, O_WRONLY | O_NONBLOCK);
	my_assert_(fifo_fd == -1 && errno == ENXIO);
	
	fcntl (fifo_fd, F_SETFL, O_WRONLY);

	char buffer[BUFFSIZE] = {0};
	
	int streaming_file = open (file_name, O_RDONLY);

	int count_symbols = -1;

	while (count_symbols != 0)
	{
		count_symbols = read (streaming_file, buffer, BUFFSIZE);
		write (fifo_fd, buffer, count_symbols);
	}

	close (fifo_fd);
	close (streaming_file);

	return 0;
}
