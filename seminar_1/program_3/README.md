## Задание

```c
	char buf[100];
	int rc;
	int fd[2];
	pipe(fd);
	if (fork() == 0) {
		dup2(fd[1], 1);
		close(fd[1]);
		close(fd[0]);
		execlp("ls", "ls", NULL);
		perror("ls");
		exit(1);
	}
	close(fd[1]);
	wait(NULL);
	while((rc = read(fd[0], buf, sizeof(buf)))>0) {
		   /* ... */
	}
```

Объяснить, как программа выше может попасть в состояние deadlock, написать исправленную версию.

## Task

```c
	char buf[100];
	int rc;
	int fd[2];
	pipe(fd);
	if (fork() == 0) {
		dup2(fd[1], 1);
		close(fd[1]);
		close(fd[0]);
		execlp("ls", "ls", NULL);
		perror("ls");
		exit(1);
	}
	close(fd[1]);
	wait(NULL);
	while((rc = read(fd[0], buf, sizeof(buf)))>0) {
		   /* ... */
	}
```

Explain the mechanism of the above program deadlocking, write correct version.
