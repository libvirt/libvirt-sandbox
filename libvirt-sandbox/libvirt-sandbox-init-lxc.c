/*
 * libvirt-sandbox-init-lxc.c: lxc guest startup
 *
 * Copyright (C) 2009-2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

/*
 * This is a crazy small init process that runs as the init
 * of the container. Its job is to decode the command line
 * and run the user specified real include.
 */

#include <config.h>

#include <glib.h>

#include <stdio.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void set_debug(void);

static int debug = 0;

int
main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    set_debug();

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-lxc: ext2 mini initrd starting up\n");

    pid_t pid = fork();
    if (pid < 0)
        goto cleanup;

    if (pid == 0) {
        struct termios  rawattr;
        tcgetattr(STDIN_FILENO, &rawattr);
        cfmakeraw(&rawattr);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawattr);

#define PATH LIBEXECDIR "/libvirt-sandbox-init-common"
#if 0
#define STRACE "/usr/bin/strace"
#define STRACE_FILTER "trace=read,write,poll,close"
#endif

#ifdef STRACE
#define BINARY STRACE
#else
#define BINARY PATH
#endif
        const char *args[] = {
#ifdef STRACE
            STRACE, "-f", "-e", STRACE_FILTER, "-s", "2000",
            PATH,
#else
            "init",
#endif
            NULL
        };
        if (debug)
            fprintf(stderr, "Running interactive\n");
        execv(BINARY, (char**)args);
        fprintf(stderr, "libvirt-sandbox-init-lxc: %s: cannot execute %s: %s\n",
                __func__, BINARY, strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        int status;
        do {
            if (waitpid(pid, &status, WUNTRACED | WCONTINUED) < 0) {
                fprintf(stderr, "libvirt-sandbox-init-lxc: %s: cannot wait for %d: %s\n",
                        __func__, pid, strerror(errno));
                exit(EXIT_FAILURE);
            }

            if (debug) {
                if (WIFEXITED(status)) {
                    fprintf(stderr, "exited, status=%d\n", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    fprintf(stderr, "killed by signal %d\n", WTERMSIG(status));
                } else if (WIFSTOPPED(status)) {
                    fprintf(stderr, "stopped by signal %d\n", WSTOPSIG(status));
                } else if (WIFCONTINUED(status)) {
                    fprintf(stderr, "continued\n");
                }
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

cleanup:
    exit(EXIT_FAILURE);
}


static void set_debug(void)
{
    const char *env = getenv("LIBVIRT_LXC_CMDLINE");

    if (env &&
        strstr(env, "debug"))
        debug=1;
}
