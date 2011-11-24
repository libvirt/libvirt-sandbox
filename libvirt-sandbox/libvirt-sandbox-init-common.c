/*
 * Copyright (C) 2011 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#if HAVE_CAPNG
#include <cap-ng.h>
#endif
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <pty.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <grp.h>

#define MAGIC "xoqpuɐs"

static ssize_t
safewrite(int fd, const void *buf, size_t count)
{
    size_t nwritten = 0;
    while (count > 0) {
        ssize_t r = write(fd, buf, count);

        if (r < 0 && errno == EINTR)
            continue;
        if (r < 0)
            return r;
        if (r == 0)
            return nwritten;
        buf = (const char *)buf + r;
        count -= r;
        nwritten += r;
    }
    return nwritten;
}


static int
start_dbus_uuidgen(void)
{
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    const char *dbusargv[] = { "/bin/dbus-uuidgen", "--ensure", NULL };
    int null = open("/dev/null", O_RDWR);

    if (null < 0) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open /dev/null: %s\n",
	      __func__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);

    close(null);

    execv(dbusargv[0], (char**)dbusargv);
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
	    __func__, dbusargv[0], strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    while (1) {
      int ret = waitpid(pid, NULL, 0);
      if (ret == -1 || ret == pid)
	break;
    }
  }

  return 0;
}

static int
start_dbus(void)
{
  pid_t pid;
  if (mount("none", "/var/lib/dbus", "tmpfs", 0, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot mount tmpfs on /var/lib/dbus: %s\n",
	    __func__, strerror(errno));
    return -1;
  }
  if (mount("none", "/var/run", "tmpfs", 0, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot mount tmpfs on /var/run: %s\n",
	    __func__, strerror(errno));
    return -1;
  }
  if (mkdir("/var/run/dbus", 0755) < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot create /var/run/dbus: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (start_dbus_uuidgen() < 0)
    return -1;

  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    const char *dbusargv[] = { "/bin/dbus-daemon", "--system", "--fork", NULL };
    int null = open("/dev/null", O_RDWR);

    if (null < 0) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open /dev/null: %s\n",
	      __func__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);

    close(null);

    execv(dbusargv[0], (char **)dbusargv);
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
	    __func__, dbusargv[0], strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    const char *path = "/var/run/dbus/system_bus_socket";
    struct stat sb;
    int count = 0;
    while (1) {
      int ret = waitpid(pid, NULL, 0);
      if (ret == -1 || ret == pid)
	break;
    }
  retry:
    if (stat(path, &sb) < 0) {
      if (errno == ENOENT && count < 50) {
	count++;
	usleep(50*1000ll);
	goto retry;
      } else {
	fprintf(stderr, "libvirt-sandbox-init-common: %s: socket %s did not show up: %s\n",
		__func__, path, strerror(errno));
	return -1;
      }
    }
  }

  return 0;
}

static int
start_consolekit(void)
{
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    const char *ckitargv[] = { "/usr/sbin/console-kit-daemon", NULL };
    int null = open("/dev/null", O_RDWR);

    if (null < 0) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open /dev/null: %s\n",
	      __func__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);

    close(null);

    execv(ckitargv[0], (char**)ckitargv);
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
	    __func__, ckitargv[0], strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    while (1) {
      int ret = waitpid(pid, NULL, 0);
      if (ret == -1 || ret == pid)
	break;
    }
  }

  return 0;
}

static int
start_xorg(void)
{
  pid_t pid;
  if (mount("none", "/var/log", "tmpfs", 0, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot mount tmpfs on /var/log: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (mount("none", "/var/lib/xkb", "tmpfs", 0, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot mount tmpfs on /var/lib/xkb: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    const char *xorgargv[] = { "/usr/bin/Xorg", NULL };
    int null = open("/dev/null", O_RDWR);

    if (null < 0) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open /dev/null: %s\n",
	      __func__, strerror(errno));
      exit(EXIT_FAILURE);
    }
    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);

    close(null);

    execv(xorgargv[0], (char**)xorgargv);
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
	    __func__, xorgargv[0], strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    const char *path = "/tmp/.X11-unix/X0";
    struct stat sb;
    int count = 0;
  retry:
    if (stat(path, &sb) < 0) {
      if (errno == ENOENT && count < 50) {
	count++;
	usleep(50*1000ll);
	goto retry;
      } else {
	fprintf(stderr, "libvirt-sandbox-init-common: %s: socket %s did not show up: %s\n",
		__func__, path, strerror(errno));
	return -1;
      }
    }
  }

  setenv("DISPLAY", ":0.0", 1);
  return 0;
}


static int
start_windowmanager(const char *path)
{
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    return -1;
  }

  if (pid == 0) {
    char *wmargv[] = { (char*)path, NULL };
    int null = open("/dev/null", O_RDWR);

    if (null < 0) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot open /dev/null: %s\n",
	      __func__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    dup2(null, STDIN_FILENO);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);

    close(null);

    execv(wmargv[0], wmargv);
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot execute %s: %s\n",
	    __func__, wmargv[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  return 0;
}


static int
decode_command(const char *base64str, char ***retargv)
{
    gsize len;
    gchar *cmdstr;
    int theargc = 0;
    char **theargv = NULL;
    char *end;
    long long int ret;
    char *cmd;
    char **tmp;

  if (!(cmdstr = (char*)g_base64_decode(base64str, &len))) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot decode %s\n",
	    __func__, base64str);
    return -1;
  }

  ret = strtoll(cmdstr, &end, 10);
  if ((ret == LLONG_MIN || ret == LLONG_MAX) && errno == ERANGE) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot read length of %s\n",
	    __func__, base64str);
    return -1;
  }

  cmd = end + 1;
  do {
    tmp = realloc(theargv, sizeof(char *) * theargc+1);
    if (!tmp) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: %s\n",
	      __func__, strerror(errno));
      return -1;
    }
    theargv = tmp;
    if (!(theargv[theargc] = strdup(cmd))) {
      fprintf(stderr, "libvirt-sandbox-init-common: %s: %s\n",
	      __func__, strerror(errno));
      return -1;
    }

    cmd += strlen(cmd) + 1;
    theargc++;
  } while (cmd < (end + 1 + ret));

  tmp = realloc(theargv, sizeof(char *) * theargc+1);
  if (!tmp) {
    fprintf(stderr, "libvirt-sandbox-init-common: %s: %s\n",
	    __func__, strerror(errno));
    return -1;
  }
  theargv = tmp;
  theargv[theargc++] = NULL;

  *retargv = theargv;
  return 0;
}


static int change_user(const char *user, uid_t uid, gid_t gid, const char *home)
{
  if (strcmp(user, "root") == 0)
    return 0;

  setenv("HOME", home, 1);
  setenv("USER", user, 1);
  setenv("USERNAME", user, 1);
  setenv("LOGNAME", user, 1);

#if HAVE_CAPNG
  capng_clear(CAPNG_SELECT_BOTH);
  if (capng_change_id(uid, gid,
		      CAPNG_DROP_SUPP_GRP | CAPNG_CLEAR_BOUNDING)) {
    fprintf(stderr, "Cannot change ID to %d:%d: %s\n",
	    uid, gid, strerror(errno));
    return -1;
  }
#else
  if (setgroups(0, NULL) < 0) {
    fprintf(stderr, "Cannot clear supplementary groups IDs: %s\n",
	    strerror(errno));
    return -1;
  }
  if (setregid(gid, gid) < 0) {
    fprintf(stderr, "Cannot change group ID to %d: %s\n",
	    gid, strerror(errno));
    return -1;
  }
  if (setreuid(uid, uid) < 0) {
    fprintf(stderr, "Cannot change user ID to %d: %s\n",
	    uid, strerror(errno));
    return -1;
  }
#endif

  if (chdir(home) < 0) {
    if (chdir("/") < 0) {
      fprintf(stderr, "Cannot change working directory to %s or /: %s",
	      home, strerror(errno));
      return -1;
    }
  }

  return 0;
}

static int run_command(int interactive, char **argv,
		       int *childin, int *childout)
{
  int pid;
  int master = -1;
  int slave = -1;
  int pipein[2] = { -1, -1};
  int pipeout[2] = { -1, -1};

  *childin = *childout = -1;

  if (interactive) {
    if (openpty(&master, &slave, NULL, NULL, NULL) < 0) {
      fprintf(stderr, "Cannot setup slave for child: %s\n",
	      strerror(errno));
      goto error;
    }

    *childin = master;
    *childout = master;
  } else {
    if (pipe(pipein) < 0 ||
	pipe(pipeout) < 0) {
      fprintf(stderr, "Cannot open pipe for child: %s\n",
	      strerror(errno));
      goto error;
    }
    *childin = pipein[1];
    *childout = pipeout[0];
  }

  if ((pid = fork()) < 0) {
    fprintf(stderr, "Cannot fork child %s\n", strerror(errno));
    goto error;
  }

  if (pid == 0) {
    if (interactive) {
      close(master);
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);

      if (dup2(slave, STDIN_FILENO) != STDIN_FILENO)
	abort();
      if (dup2(slave, STDOUT_FILENO) != STDOUT_FILENO)
	abort();
      if (dup2(slave, STDERR_FILENO) != STDERR_FILENO)
	abort();

      if (setsid() < 0) {
	fprintf(stderr, "Cannot start new session ID: %s\n", strerror(errno));
	abort();
      }
    } else {
      close(pipein[1]);
      close(pipeout[0]);
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);

      if (dup2(pipein[0], STDIN_FILENO) != STDIN_FILENO)
	abort();
      if (dup2(pipeout[1], STDOUT_FILENO) != STDOUT_FILENO)
	abort();
      if (dup2(pipeout[1], STDERR_FILENO) != STDERR_FILENO)
	abort();
    }

    if (safewrite(STDOUT_FILENO, MAGIC, sizeof(MAGIC)) != sizeof(MAGIC)) {
      fprintf(stderr, "Cannot send handshake magic %s\n", MAGIC);
      abort();
    }

    execv(argv[0], argv);
    fprintf(stderr, "Cannot execute '%s': %s\n", argv[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (interactive)
    close(slave);
  else {
    close(pipein[0]);
    close(pipeout[1]);
  }

  return pid;

 error:
  if (interactive) {
    if (master != -1)
      close(master);
    if (slave != -1)
      close(slave);
  } else {
    if (pipein[0] < 0)
      close(pipein[0]);
    if (pipein[1] < 0)
      close(pipein[1]);
    if (pipeout[0] < 0)
      close(pipeout[0]);
    if (pipeout[1] < 0)
      close(pipeout[1]);
  }
  *childin = *childout = -1;
  return -1;
}

static ssize_t read_data(int fd, char *buf, size_t *len, size_t max)
{
  size_t avail = max - *len;
  ssize_t got;

  got = read(fd, buf + *len, avail);
  if (got < 0) {
    if (errno == EAGAIN)
      return 0;
    fprintf(stderr, "Unable to read data: %s\n", strerror(errno));
    return -1;
  }

  *len += got;

  return got;
}

static ssize_t write_data(int fd, char *buf, size_t *len)
{
  ssize_t got;

  got = write(fd, buf, *len);
  if (got < 0) {
    if (errno == EAGAIN)
      return 0;
    fprintf(stderr, "Unable to write data: %s\n", strerror(errno));
    return -1;
  }

  memmove(buf, buf+got, *len-got);
  *len -= got;

  return got;
}


static int run_io(int *childin, int *childout)
{
  char inbuf[1024];
  size_t inlen = 0;
  char outbuf[1024];
  size_t outlen = 0;

  int parentin = STDIN_FILENO;
  int parentout = STDOUT_FILENO;

  int inescape = 0;
  int outescape = 0;

  while (1) {
    int i;
    struct pollfd fds[6];
    int nfds = 0;

    if (parentin != -1) {
      if (inlen < sizeof(inbuf)) {
	fds[nfds].fd = parentin;
	fds[nfds].events = POLLIN;
	nfds++;
      }
    }
    if (parentout != -1) {
      if (outlen && !outescape) {
	fds[nfds].fd = parentout;
	fds[nfds].events = POLLOUT;
	nfds++;
      }
    }
    if (*childin != -1 && *childin == *childout) {
      fds[nfds].events = 0;
      if (inlen && !inescape)
	fds[nfds].events |= POLLOUT;
      if (outlen < sizeof(outbuf))
	fds[nfds].events |= POLLIN;
      if ((inlen && !inescape) || (outlen < sizeof(outbuf))) {
	fds[nfds].fd = *childin;
	nfds++;
      }
    } else if (*childin != -1) {
      if (inlen && !inescape) {
	fds[nfds].fd = *childin;
	fds[nfds].events |= POLLOUT;
	nfds++;
      }
    } else if (*childout != -1) {
      if (outlen < sizeof(outbuf)) {
	fds[nfds].fd = *childout;
	fds[nfds].events = POLLIN;
	nfds++;
      }
    }

    if ((nfds = poll(fds, nfds, -1)) < 0) {
      fprintf(stderr, "Poll error:%s\n",
	      strerror(errno));
      return -1;
    }

    for (i = 0 ; i < (sizeof(fds)/sizeof(fds[0])) ; i++) {
      ssize_t got;

      if (fds[i].fd == parentin) {
	if (fds[i].revents & (POLLIN|POLLHUP)) {
	  if (fds[i].revents & POLLIN) {
	    got = read_data(parentin, inbuf, &inlen, sizeof(inbuf));
	    if (got < 0)
	      return -1;
	  } else {
	    got = 0;
	  }
	  if (got == 0) {
	    close(parentin);
	    parentin = -1;
	    if (inlen == 0) {
	      close(*childin);
	      if (*childin == *childout) {
		*childout = -1;
		if (outlen == 0) {
		  close(parentout);
		  parentout = -1;
		}
	      }
	      *childin = -1;
	    }
	  }
	}
      } else if (fds[i].fd == parentout) {
	if (fds[i].revents & POLLOUT) {
	  got = write_data(parentout, outbuf, &outlen);
	  if (got < 0)
	    return -1;
	  if (outlen == 0 && *childout == -1) {
	    close(parentout);
	    parentout = -1;
	  }
	}
      } else if (fds[i].fd == *childin) {
        if (fds[i].revents & POLLOUT) {
	  got = write_data(*childin, inbuf, &inlen);
	  if (got < 0)
	    return -1;
	  if (inlen == 0 && parentin == -1) {
	    close(*childin);
	    if (*childin == *childout) {
	      *childout = -1;
	      if (outlen == 0) {
		close(parentout);
		parentout = -1;
	      }
	    }
	    *childin = -1;
	  }
	} else if (*childin == *childout &&
		   (fds[i].revents & (POLLIN|POLLHUP))) {
	  if (fds[i].revents & POLLIN) {
	    got = read_data(*childout, outbuf, &outlen, sizeof(outbuf));
	    if (got < 0)
	      return -1;
	  } else {
	    got = 0;
	  }
	  if (got == 0) {
	    close(*childout);
	    if (*childin == *childout)
	      *childin = -1;
	    *childout = -1;
	    close(parentin);
	    parentin = -1;
	    if (outlen == 0) {
	      close(parentout);
	      parentout = -1;
	    }
	  }
	}
      } else if (fds[i].fd == *childout) {
	if (fds[i].revents & (POLLIN|POLLHUP)) {
	  if (fds[i].revents & POLLIN) {
	    got = read_data(*childout, outbuf, &outlen, sizeof(outbuf));
	    if (got < 0)
	      return -1;
	  } else {
	    got = 0;
	  }
	  if (got == 0) {
	    close(*childout);
	    if (*childin == *childout)
	      *childin = -1;
	    *childout = -1;
	    close(parentin);
	    parentin = -1;
	    if (outlen == 0) {
	      close(parentout);
	      parentout = -1;
	    }
	  }
	}
      }
    }

    if (1) {
    int n;
    for (n = 0, i = 0 ; i < inlen ; i++) {
      if (inescape) {
	if (inbuf[i] == '9') {
	  close(parentin);
	  parentin = -1;
	} else {
	  inbuf[n] = inbuf[i];
	  n++;
	}
	inescape = 0;
      } else {
	if (inbuf[i] == '\\') {
	  inescape = 1;
	} else {
	  if (n < i)
	    inbuf[n] = inbuf[i];
	  n++;
	}
      }
    }
    inlen = n;
    }

    if (parentin == -1 &&
	parentout == -1 &&
	*childout == -1 &&
	*childin == -1)
      break;
  }
  return 0;
}


int main(int argc, char **argv) {
  int debug = 0;
  int interactive = 0;
  int xorg = 0;
  const char *user = "nobody";
  const char *wm = NULL;
  int pid;
  int err;
  int childin;
  int childout;
  char **childargv;
  const char *cmdarg;
  int uid = 99;
  int gid = 99;
  const char *home = "/home/nobody";
  long int tmp;

  while (1) {
    int option_index = 0;
    int c;
    static const struct option long_options[] = {
      {"debug", no_argument, NULL,  'd' },
      {"user",  required_argument, NULL,  'n' },
      {"uid",  required_argument, NULL,  'u' },
      {"gid",  required_argument, NULL,  'g' },
      {"home",  required_argument, NULL,  'h' },
      {"interactive", no_argument, NULL, 'i' },
      {"xorg", no_argument, NULL, 'X' },
      {"windowmanager",  required_argument, NULL,  'w' },
      {NULL, 0, NULL,  0 }
    };

    c = getopt_long(argc, argv, "diXu:g:n:h:w:",
		    long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'd':
      debug = 1;
      break;

    case 'i':
      interactive = 1;
      break;

    case 'X':
      xorg = 1;
      break;

    case 'n':
      user = optarg;
      break;

    case 'u':
      tmp = strtoll(optarg, NULL, 10);
      if ((tmp == LLONG_MIN || tmp == LLONG_MAX) && errno == ERANGE) {
	fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot parse uid: %s\n",
		__func__, optarg);
	exit(EXIT_FAILURE);
      }
      uid = (uid_t)tmp;
      break;

    case 'g':
      tmp = strtoll(optarg, NULL, 10);
      if ((tmp == LLONG_MIN || tmp == LLONG_MAX) && errno == ERANGE) {
	fprintf(stderr, "libvirt-sandbox-init-common: %s: cannot parse gid: %s\n",
		__func__, optarg);
	exit(EXIT_FAILURE);
      }
      gid = (uid_t)tmp;
      break;

    case 'h':
      home = optarg;
      break;

    case 'w':
      wm = optarg;
      break;

    default:
      fprintf(stderr, "Unexpected command line argument\n");
      exit(EXIT_FAILURE);
    }
  }

  if (optind == argc) {
    fprintf(stderr, "Missing command to execute\n");
    exit(EXIT_FAILURE);
  }

  cmdarg = argv[optind];

  setenv("PATH", "/bin:/usr/bin:/usr/local/bin:/sbin/:/usr/sbin", 1);
  struct rlimit res = { 65536, 65536 };
  setrlimit(RLIMIT_NOFILE, &res);

  if (xorg &&
      start_dbus() < 0)
    exit(EXIT_FAILURE);

  if (xorg &&
      start_consolekit() < 0)
    exit(EXIT_FAILURE);

  if (xorg &&
      start_xorg() < 0)
    exit(EXIT_FAILURE);

  if (change_user(user, uid, gid, home) < 0)
    exit(EXIT_FAILURE);

  if (wm &&
      start_windowmanager(wm) < 0)
    exit(EXIT_FAILURE);

  if (decode_command(cmdarg, &childargv) < 0)
    exit(EXIT_FAILURE);

  if ((pid = run_command(interactive, childargv,
			 &childin, &childout)) < 0)
    exit(EXIT_FAILURE);

  if (debug) {
    fprintf(stderr, "Got %d %d\n",
	    childin, childout);
  }

  err = run_io(&childin, &childout);

  if (childin != -1)
    close(childin);
  if (childout != -1 && childout != childin)
    close(childout);

  while (waitpid(pid, NULL, 0) != pid);

  exit(err < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
