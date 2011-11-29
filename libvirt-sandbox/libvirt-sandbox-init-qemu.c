/*
 * libvirt-sandbox-init-qemu.c: qemu guest startup
 *
 * Copyright (C) 2009-2011 Red Hat
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
 * This is a crazy small init process that runs inside the
 * initrd of the sandbox. Its job is to mount the virtio
 * 9p filesystem(s) from the host and hand control to the
 * user specified program as quickly as possible.
 */

#include <config.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/reboot.h>
#include <termios.h>

#define ATTR_UNUSED __attribute__((__unused__))

static void print_uptime (void);
static void insmod (const char *filename);
static char *get_cmd_and_args(void);
static char *get_user_name(void);
static char *get_user_id(void);
static char *get_group_id(void);
static char *get_home_dir(void);
static int get_isatty(void);
static int get_xorg(void);
static char *get_windowmanager(void);
static void set_debug(void);

static int debug = 0;
static char line[1024];

static void exit_poweroff(void) __attribute__((noreturn));

static void exit_poweroff(void)
{
  sync();
  reboot(RB_POWER_OFF);
  perror("reboot");
  abort();
}

static void sig_child(int signum ATTR_UNUSED)
{
  int status;
  while (1) {
    int ret = waitpid(-1, &status, WNOHANG);
    if (ret == -1 || ret == 0)
      break;
  }
}

static void
mount_mkdir(const char *dir, int mode);

static void
mount_mkparent(const char *dir, int mode)
{
  char *tmp = strrchr(dir, '/');
  if (tmp && tmp != dir) {
    char *parent = strndup(dir, tmp - dir);
    mount_mkdir(parent, mode);
    free(parent);
  }
}

static void
mount_mkdir(const char *dir, int mode)
{
  mount_mkparent(dir, 0755);

  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s (mode=%o)\n", __func__, dir, mode);

  if (mkdir(dir, mode) < 0) {
    if (errno != EEXIST) {
      fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot make directory %s: %s\n",
	      __func__, dir, strerror(errno));
      exit_poweroff();
    }
  }
}

#if 0
static void
mount_mkfile(const char *file, int mode)
{
  int fd;

  mount_mkparent(file, 0755);

  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s (mode=%o)\n", __func__, file, mode);

  if ((fd = open(file, O_CREAT|O_WRONLY, mode)) < 0 &&
      errno != EEXIST) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open file %s: %s\n",
	    __func__, dir, strerror(errno));
    exit_poweroff();
  }
  close(fd);
}
#endif

static void
mount_other_opts(const char *dst, const char *type, const char *opts, int mode)
{
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s (type=%s opts=%s)\n", __func__, dst, type, opts);

  mount_mkdir(dst, mode);

  if (mount("none", dst, type, 0, opts) < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount %s on %s (%s:%s): %s\n",
	    __func__, type, dst, type, opts, strerror(errno));
    exit_poweroff();
  }

  if (chmod(dst, mode) < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot set directory mode %s: %s\n",
	    __func__, dst, strerror(errno));
    exit_poweroff();
  }
}

static void
mount_other(const char *dst, const char *type, int mode)
{
  mount_other_opts(dst, type, "", mode);
}

static void
mount_9pfs(const char *src, const char *dst, int mode, int readonly)
{
  int flags = 0;
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s -> %s (%d)\n", __func__, src, dst, readonly);

  mount_mkdir(dst, mode);

  if (readonly)
    flags |= MS_RDONLY;

  if (mount(src, dst, "9p", flags, "trans=virtio") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount %s on %s (9p): %s\n",
	    __func__, src, dst, strerror(errno));
    exit_poweroff();
  }
}

#if 0
static void
bind_dir(const char *src, const char *dst, int mode)
{
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s -> %s\n", __func__, src, dst);

  mount_mkdir(dst, mode);

  if (mount(src, dst, "", MS_BIND, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot bind %s to %s: %s\n",
	    __func__, src, dst, strerror(errno));
    exit_poweroff();
  }
}

static void
bind_file(const char *src, const char *dst, int mode)
{
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s -> %s\n", __func__, src, dst);

  mount_mkfile(dst, mode);

  if (mount(src, dst, "", MS_BIND, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot bind %s to %s: %s\n",
	    __func__, src, dst, strerror(errno));
    exit_poweroff();
  }
}
#endif

int
main(int argc ATTR_UNUSED, char **argv ATTR_UNUSED)
{
  int istty, xorg;
  set_debug();

  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: ext2 mini initrd starting up\n");

  mount_other("/sys", "sysfs", 0755);

  FILE *fp = fopen("/modules", "r");
  if (fp == NULL) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open /modules: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }
  while (fgets(line, sizeof line, fp)) {
    size_t n = strlen(line);
    if (n > 0 && line[n-1] == '\n')
      line[--n] = '\0';
    insmod(line);
  }
  fclose(fp);

  if (umount("/sys") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot unmount /sys: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  /* Mount new root and chroot to it. */
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: mounting new root on /tmproot\n");

  mount_9pfs("sandbox:root", "/tmproot", 0755, 1);

  /* Note that pivot_root won't work.  See the note in
   * Documentation/filesystems/ramfs-rootfs-initramfs.txt
   * We could remove the old initramfs files, but let's not bother.
   */
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: chroot\n");

  if (chroot("/tmproot") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot chroot to /tmproot: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  if (chdir("/") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot chdir to /: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  /* Main special filesystems */
  mount_other("/dev", "tmpfs", 0755);
  mount_other_opts("/dev/pts", "devpts", "gid=5,mode=620,ptmxmode=000", 0755);
  mount_other("/root", "tmpfs", 0755);
  mount_other("/sys", "sysfs", 0755);
  mount_other("/proc", "proc", 0755);
  mount_other("/selinux", "selinuxfs", 0755);
  mount_other("/dev/shm", "tmpfs", 01777);

  char *cmdargs = get_cmd_and_args();
  char *user = get_user_name();
  char *uid = get_user_id();
  char *gid = get_group_id();
  char *home = get_home_dir();
  char *wm = get_windowmanager();

  /* Find all other virtio 9p PCI devices in sysfs */
#define SYS_9PDRIVER "/sys/bus/virtio/drivers/9pnet_virtio"
  DIR *dh = opendir(SYS_9PDRIVER);
  if (!dh) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open %s: %s\n",
	    __func__, SYS_9PDRIVER, strerror(errno));
    exit_poweroff();
  }

  struct dirent *de;
  while ((de = readdir(dh))) {
    if (strncmp(de->d_name, "virtio", 6) == 0) {
      char *mount_tag;
      if (asprintf(&mount_tag, "%s/%s/mount_tag", SYS_9PDRIVER, de->d_name) < 0) {
	fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot format mount tag: %s\n",
		__func__, strerror(errno));
	exit_poweroff();
      }

      FILE *mt = fopen(mount_tag, "r");
      if (!mt) {
	fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open %s: %s\n",
		__func__, mount_tag, strerror(errno));
	exit_poweroff();
      }
      char *tmp = NULL;
      size_t n;
      if (getline(&tmp, &n, mt) <= 0) {
	fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot read line from %s: %s\n",
		__func__, mount_tag, strerror(errno));
	exit_poweroff();
      }

      if (strcmp(tmp, "sandbox:tmp") == 0) {
	mount_9pfs(tmp, "/tmp", 0755, 0);
      } else if (strcmp(tmp, "sandbox:home") == 0) {
	mount_9pfs(tmp, home, 0755, 0);
      }
      fclose(mt);
      free(tmp);
      free(mount_tag);
    }
  }

  if (mkdir("/dev/input", 0777) < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot make directory /dev/input: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

#define MKNOD(file, mode, dev)						\
  do {									\
    if (mknod(file, mode, dev) < 0) {					\
      fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot make dev %s %llu: %s\n", \
	      __func__, file, (unsigned long long)dev, strerror(errno)); \
      exit_poweroff();							\
    }									\
  } while (0)

  umask(0000);
  MKNOD("/dev/null", S_IFCHR |0666, makedev(1, 3));
  MKNOD("/dev/zero", S_IFCHR |0666, makedev(1, 5));
  MKNOD("/dev/full", S_IFCHR |0666, makedev(1, 7));
  MKNOD("/dev/random", S_IFCHR |0666, makedev(1, 8));
  MKNOD("/dev/urandom", S_IFCHR |0666, makedev(1, 9));
  MKNOD("/dev/console", S_IFCHR |0700, makedev(5, 1));
  MKNOD("/dev/tty", S_IFCHR |0777, makedev(5, 0));
  MKNOD("/dev/tty0", S_IFCHR |0700, makedev(4, 0));
  MKNOD("/dev/tty1", S_IFCHR |0700, makedev(4, 1));
  MKNOD("/dev/tty2", S_IFCHR |0700, makedev(4, 2));
  MKNOD("/dev/ttyS0", S_IFCHR |0777, makedev(4, 64));
  MKNOD("/dev/hvc0", S_IFCHR |0777, makedev(229, 0));
  MKNOD("/dev/fb", S_IFCHR |0700, makedev(29, 0));
  MKNOD("/dev/fb0", S_IFCHR |0700, makedev(29, 0));
  MKNOD("/dev/mem", S_IFCHR |0600, makedev(1, 1));
  MKNOD("/dev/rtc", S_IFCHR |0700, makedev(254, 0));
  MKNOD("/dev/rtc0", S_IFCHR |0700, makedev(254, 0));
  MKNOD("/dev/ptmx", S_IFCHR |0777, makedev(5, 2));
  MKNOD("/dev/input/event0", S_IFCHR |0777, makedev(13, 64));
  MKNOD("/dev/input/event1", S_IFCHR |0777, makedev(13, 65));
  MKNOD("/dev/input/event2", S_IFCHR |0777, makedev(13, 66));
  MKNOD("/dev/input/mice", S_IFCHR |0777, makedev(13, 63));
  MKNOD("/dev/input/mouse0", S_IFCHR |0777, makedev(13, 32));
  umask(0022);

  /* Run /init from ext2 filesystem. */
  print_uptime();
  if (!cmdargs) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: missing encoded command + args\n",
	    __func__);
    exit_poweroff();
  }

  istty = get_isatty();
  xorg = get_xorg();

  signal(SIGCHLD, sig_child);

  pid_t pid = fork();
  if (pid < 0)
    goto cleanup;

  if (pid == 0) {
    struct termios  rawattr;
    tcgetattr(STDIN_FILENO, &rawattr);
    cfmakeraw(&rawattr);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawattr);

#if 0
#define STRACE "/usr/bin/strace"
#define STRACE_FILTER "trace=read,write,poll,close"
#endif

    const char *args[50];
    int narg = 0;
    memset(&args, 0, sizeof(args));
#ifdef STRACE
    args[narg++] = STRACE;
    args[narg++] = "-q";
    // args[narg++] = "-f";
    args[narg++] = "-e";
    args[narg++] = STRACE_FILTER;
//    args[narg++] = "-s";
//    args[narg++] = "2000";
#endif
    args[narg++] = LIBEXECDIR "/libvirt-sandbox-init-common";
    if (istty)
      args[narg++] = "-i";
    args[narg++] = "-n";
    args[narg++] = user;
    args[narg++] = "-u";
    args[narg++] = uid;
    args[narg++] = "-g";
    args[narg++] = gid;
    args[narg++] = "-h";
    args[narg++] = home;
    if (xorg) {
      args[narg++] = "-X";
      if (wm) {
	args[narg++] = "-w";
	args[narg++] = wm;
      }
    }
    if (debug)
        args[narg++] = "-d";
    args[narg++] = cmdargs;

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: Running common init %s\n", args[0]);
    execv(args[0], (char**)args);
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot execute %s: %s\n",
	    __func__, args[0], strerror(errno));
    exit_poweroff();
  } else {
    int status;
    do {
      if (waitpid(pid, &status, WUNTRACED | WCONTINUED) < 0) {
	fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot wait for %d: %s\n",
		__func__, pid, strerror(errno));
	exit_poweroff();
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
  exit_poweroff();
}

static void
insmod(const char *filename)
{
  if (debug)
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, filename);

  pid_t pid = fork ();
  if (pid < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot fork: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  if (pid == 0) { /* Child. */
    execl("/insmod.static", "insmod.static", filename, NULL);
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot run /insmod.static: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  /* Parent. */
  int status;
  if (wait(&status) < 0 ||
      WEXITSTATUS(status) != 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot wait for %d: %s\n",
	    __func__, pid, strerror(errno));
    exit_poweroff();
  }
}

/* Print contents of /proc/uptime. */
static void
print_uptime(void)
{
  if (!debug)
    return;
  FILE *fp = fopen("/proc/uptime", "r");
  if (fp == NULL) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open /proc/uptime: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  if (!fgets(line, sizeof line, fp)) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot read /proc/uptime: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }
  fclose(fp);

  fprintf(stderr, "libvirt-sandbox-init-qemu: uptime: %s", line);
}


static char *
get_command_arg(const char *name)
{
  FILE *fp = fopen("/proc/cmdline", "r");
  char *start, *end;

  if (fp == NULL) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open /proc/cmdline: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  if (!fgets(line, sizeof line, fp)) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot read /proc/cmdline: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }
  fclose(fp);

  start = strstr(line, name);
  if (!start)
    return NULL;
  start += strlen(name);
  end = strstr(start, " ");
  if (!end)
    end = strstr(start, "\n");
  if (end)
    return strndup(start, end-start);
  else
    return strdup(start);
}


static int
has_command_arg(const char *name)
{
  FILE *fp = fopen("/proc/cmdline", "r");

  if (fp == NULL) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open /proc/cmdline: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  if (!fgets(line, sizeof line, fp)) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot read /proc/cmdline: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }

  fclose(fp);

  return strstr(line, name) != NULL;
}


static char *
get_user_name(void)
{
  return get_command_arg("sandbox-user=");
}

static char *
get_user_id(void)
{
  return get_command_arg("sandbox-uid=");
}

static char *
get_group_id(void)
{
  return get_command_arg("sandbox-gid=");
}

static char *
get_home_dir(void)
{
  return get_command_arg("sandbox-home=");
}

static char *
get_cmd_and_args(void)
{
  return get_command_arg("sandbox-cmd=");
}

static int
get_isatty(void)
{
  return has_command_arg("sandbox-isatty");
}

static int
get_xorg(void)
{
  return has_command_arg("sandbox-xorg");
}

static char *
get_windowmanager(void)
{
  return get_command_arg("sandbox-wm=");
}


static void set_debug(void)
{
  mkdir("/proc", 0755);
  if (mount("none", "/proc", "proc", 0, "") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount /proc: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }
  FILE *fp = fopen("/proc/cmdline", "r");

  if (fp && fgets(line, sizeof line, fp)) {
    if (strstr(line, "debug"))
      debug=1;
    fclose(fp);
  }
  if (umount("/proc") < 0) {
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot unmount /proc: %s\n",
	    __func__, strerror(errno));
    exit_poweroff();
  }
}
