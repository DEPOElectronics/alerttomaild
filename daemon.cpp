/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
 *                                                                         *
 * This program is free software. You may use, modify, and redistribute it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation, either version 3 or (at your option)   *
 * any later version. This program is distributed without any warranty.    *
 * See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
 \*************************************************************************/

/* become_daemon.c

 A function encapsulating the steps in becoming a daemon.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "daemon.hpp"
#include <unistd.h>
#include <cstdlib>
#include <dirent.h>
//#include <stdio.h>
#include <sys/signal.h>
#include <string.h>
#include <fstream>

void SetPidFile(const char *Filename)
{
	if (Filename != NULL)
	{
		FILE *f;

		f = fopen(Filename, "w+");
		if (f)
		{
			fprintf(f, "%u", getpid());
			fclose(f);
		}
	}
}

int SetDaemon(int flags, const char *PID_FILE) /* Returns 0 on success, -1 on error */
{
	int maxfd, fd;

	switch (fork())
	{
	case -1:
	{
		return EXIT_FAILURE;
	}
	case 0:
	{
		break;
	}
	default:
	{
		_exit(EXIT_SUCCESS);
	}
	}

	if (setsid() == -1)
		return -1;

	/*int pid = fork();
	if (pid == -1)
	{  Ensure we are not session leader
		return EXIT_FAILURE;
	}*/

	if (!(flags & BD_NO_UMASK0))
		umask(0); /* Clear file mode creation mask */

	if (!(flags & BD_NO_CHDIR))
	{
		int r = chdir("/");                     // Change to root directory
		if (r != 0)
		{
			return EXIT_FAILURE;
		}
	}

	if (!(flags & BD_NO_CLOSE_FILES))
	{ /* Close all open files */
		maxfd = sysconf(_SC_OPEN_MAX);
		if (maxfd == -1) /* Limit is indeterminate... */
			maxfd = BD_MAX_CLOSE; /* so take a guess */

		for (fd = 0; fd < maxfd; fd++)
			close(fd);
	}

	if (!(flags & BD_NO_REOPEN_STD_FDS))
	{
		close(STDIN_FILENO); /* Reopen standard fd's to /dev/null */

		fd = open("/dev/null", O_RDWR);

		if (fd != STDIN_FILENO) /* 'fd' should be 0 */
			return -1;
		if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
			return -1;
		if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
			return -1;
	}
	// данная функция создаст файл с нашим PID'ом
	SetPidFile(PID_FILE);

	//int status = MonitorProc();
	return 0;
}
