#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
	char buf[100] = {0};
	int rc;
	int fd[2];
	rc = pipe(fd);
	assert((rc == 0) && "pipe");
	if (((rc = fork())) == 0) {
		rc = dup2(fd[1], 1);
		assert((rc != -1) && "dup2");
		close(fd[1]);
		close(fd[0]);
		execlp("ls", "ls", NULL);
		assert(!"ls");
	}
	assert((rc > 0) || "fork");
	close(fd[1]);
	/* The reason of the deadlock was that
	   write(), called by 'ls' is blocking,
	   so in case of insufficient buffer
	   size child process blocked on write()
	   while parent waited for it to die.
	   Solution is to change places of read()
	   and wait() calls. */
	while ((rc = read(fd[0], buf, sizeof(buf))) > 0)
		write(STDOUT_FILENO, buf, rc);

	rc = wait(NULL);
	assert((rc != -1) && "wait");

	return 0;
}
