/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/

/* daemon.hpp

   Header file for daemon.cpp.
*/
#ifndef DAEMON_H /* Prevent double inclusion */
#define DAEMON_H

/* Bit-mask values for 'flags' argument of becomeDaemon() */

#define BD_NO_CHDIR 01       /* Don't chdir("/") */
#define BD_NO_CLOSE_FILES 02 /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS                                                   \
    04                   /* Don't reopen stdin, stdout, and                    \
                            stderr to /dev/null */
#define BD_NO_UMASK0 010 /* Don't do a umask(0) */

#define BD_MAX_CLOSE                                                           \
    8192 /* Maximum file descriptors to close if                               \
            sysconf(_SC_OPEN_MAX) is indeterminate */

int SetDaemon(int flags, const char* PID_FILE);

#endif
