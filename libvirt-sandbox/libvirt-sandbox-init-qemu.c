/*
 * libvirt-sandbox-init-qemu.c: qemu guest startup
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#include <sys/reboot.h>
#include <termios.h>
#if WITH_LZMA
#include <lzma.h>
#endif /* WITH_LZMA */
#if WITH_ZLIB
#include <zlib.h>
#endif /* WITH_ZLIB */

#define ATTR_UNUSED __attribute__((__unused__))

#define STREQ(x,y) (strcmp(x,y) == 0)
#define STRNEQ(x,y) (strcmp(x,y) != 0)

static void print_uptime (void);
static void insmod (const char *filename);
static void set_debug(void);
static int has_command_arg(const char *name,
                           char **val);

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

static void
mount_mkfile(const char *file, int mode)
{
    int fd;

    mount_mkparent(file, 0755);

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s (mode=%o)\n",
                __func__, file, mode);

    if ((fd = open(file, O_CREAT|O_WRONLY, mode)) < 0 &&
        errno != EEXIST) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open file %s: %s\n",
                __func__, file, strerror(errno));
        exit_poweroff();
    }
    if (fd != -1)
        close(fd);
}

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

    if (mount(src, dst, "9p", flags, "trans=virtio,version=9p2000.u") < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount %s on %s (9p): %s\n",
                __func__, src, dst, strerror(errno));
        exit_poweroff();
    }
}


static void
mount_entry(const char *source,
            const char *target,
            const char *type,
            const char * opts)
{
    int flags = 0;

    if (STREQ(type, "")) {
        struct stat st;
        type = NULL;
        flags |= MS_BIND;
        if (stat(source, &st) < 0) {
            fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot read mount source %s: %s\n",
                    __func__, source, strerror(errno));
            exit_poweroff();
        }
        if (S_ISDIR(st.st_mode))
            mount_mkdir(target, 755);
        else
            mount_mkfile(target, 644);
    } else {
        if (STREQ(type, "tmpfs"))
            flags |= MS_NOSUID | MS_NODEV;

        mount_mkdir(target, 0755);
    }

    if (mount(source, target, type, flags, opts) < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount %s on %s (%s, %s): %s\n",
                __func__, source, target, type, opts, strerror(errno));
        exit_poweroff();
    }
}

static void
mount_root(const char *path)
{
    int foundRoot = 0;

    /* Loop over mounts.cfg to see if we have a candidate for / */
    mount_mkdir(SANDBOXCONFIGDIR, 0755);
    mount_9pfs("sandbox:config", SANDBOXCONFIGDIR, 0755, 1);

    FILE *fp = fopen(SANDBOXCONFIGDIR "/mounts.cfg", "r");
    while (fgets(line, sizeof line, fp) && !foundRoot) {
        char *source = line;
        char *target = strchr(source, '\t');
        *target = '\0';
        target++;
        char *type = strchr(target, '\t');
        *type = '\0';
        type++;
        char *opts = strchr(type, '\t');
        *opts = '\0';
        opts++;
        char *tmp = strchr(opts, '\n');
        *tmp = '\0';

        if (STREQ(target, "/")) {
            int needsDev = strncmp(source, "/dev/", 5) == 0;

            if (debug)
                fprintf(stderr, "libvirt-sandbox-init-qemu: found root from %s\n",
                        source);

            /* In this case, we need to have a /dev before the chroot */
            if (needsDev) {
                mount_other("/proc", "proc", 0755);
                mount_other("/dev", "devtmpfs", 0755);
            }

            mount_entry(source, path, type, opts);

            if (needsDev) {
                if (umount("/dev") < 0) {
                    fprintf(stderr,
                            "libvirt-sandbox-init-qemu: %s: "
                            "cannot unmount temporary /dev: %s\n",
                            __func__, strerror(errno));
                    exit_poweroff();
                }
                if (umount("/proc") < 0) {
                    fprintf(stderr,
                            "libvirt-sandbox-init-qemu: %s: "
                            "cannot unmount temporary /proc: %s\n",
                            __func__, strerror(errno));
                    exit_poweroff();
                }
            }
            foundRoot = 1;
        }
    }
    fclose(fp);

    if (umount(SANDBOXCONFIGDIR) < 0) {
        fprintf(stderr,
                "libvirt-sandbox-init-qemu: %s: "
                "cannot unmount temporary %s: %s\n",
                __func__, SANDBOXCONFIGDIR, strerror(errno));
        exit_poweroff();
    }

    /* If we couldn't get a / in the mounts, then use the host one */
    if (!foundRoot)
        mount_9pfs("sandbox:root", path, 0755, 1);
}

int
main(int argc ATTR_UNUSED, char **argv ATTR_UNUSED)
{
    const char *args[50];
    int narg = 0;
    char *strace = NULL;

    if (getpid() != 1) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: must be run as the 'init' program of a KVM guest\n");
        exit(EXIT_FAILURE);
    }

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

    mount_root("/tmproot");

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
    mount_other("/dev", "devtmpfs", 0755);
    if (symlink("/proc/self/fd", "/dev/fd") < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: failed to create /dev/fd symlink: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }

    mount_other_opts("/dev/pts", "devpts", "gid=5,mode=620,ptmxmode=000", 0755);
    mount_other("/sys", "sysfs", 0755);
    mount_other("/proc", "proc", 0755);
    //mount_other("/selinux", "selinuxfs", 0755);
    mount_other("/dev/shm", "tmpfs", 01777);

    umask(0022);
    mount_9pfs("sandbox:config", SANDBOXCONFIGDIR, 0755, 1);

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: setting up filesystem mounts\n",
                __func__);
    fp = fopen(SANDBOXCONFIGDIR "/mounts.cfg", "r");
    if (fp == NULL) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open " SANDBOXCONFIGDIR "/mounts.cfg: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }
    while (fgets(line, sizeof line, fp)) {
        char *source = line;
        char *target = strchr(source, '\t');
        *target = '\0';
        target++;
        char *type = strchr(target, '\t');
        *type = '\0';
        type++;
        char *opts = strchr(type, '\t');
        *opts = '\0';
        opts++;
        char *tmp = strchr(opts, '\n');
        *tmp = '\0';

        if (debug)
            fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s -> %s (%s, %s)\n",
                    __func__, source, target, type, opts);

        if (STRNEQ(target, "/"))
            mount_entry(source, target, type, opts);
    }
    fclose(fp);


    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: preparing to launch common init\n",
                __func__);
    /* Run /init from ext2 filesystem. */
    print_uptime();

    signal(SIGCHLD, sig_child);

    memset(&args, 0, sizeof(args));
    if (has_command_arg("strace=", &strace) == 0) {
        args[narg++] = "/usr/bin/strace";
        args[narg++] = "-q";
        args[narg++] = "-o";
        args[narg++] = "/tmp/sandbox.log";
        args[narg++] = "-f";
        args[narg++] = "-ff";
        if (strace && STRNEQ(strace, "1")) {
            args[narg++] = "-e";
            args[narg++] = strace;
        }
        args[narg++] = "-s";
        args[narg++] = "1000";
    }

    args[narg++] = SANDBOXCONFIGDIR "/.libs/ld.so";
    args[narg++] = SANDBOXCONFIGDIR "/.libs/libvirt-sandbox-init-common";
    args[narg++] = "--poweroff";
    if (debug)
        args[narg++] = "-d";

    if (setenv("LD_LIBRARY_PATH", SANDBOXCONFIGDIR "/.libs", 1) < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot set LD_LIBRARY_PATH: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }


    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: Running common init %s\n", args[0]);
    execv(args[0], (char**)args);
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot execute %s: %s\n",
            __func__, args[0], strerror(errno));
    exit_poweroff();
}

/* This is a function exported by glibc, but not in any header :-) */
extern long init_module(const void *mem, unsigned long len, const char *args);

#define READ_SIZE (1024 * 16)

static char *readall(const char *filename, size_t *len)
{
    char *data = NULL, *tmp;
    int fd;
    size_t capacity;
    size_t offset;
    ssize_t got;

    *len = capacity = offset = 0;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot open %s\n",
                __func__, filename);
        exit_poweroff();
    }

    for (;;) {
        if ((capacity - offset) < 1024) {
            if (!(tmp = realloc(data, capacity + 2048))) {
                fprintf(stderr, "libvirt-sandbox-init-qemu: %s: out of memory\n",
                        __func__);
                exit_poweroff();
            }
            data = tmp;
            capacity += 2048;
        }

        if ((got = read(fd, data + offset, capacity - offset)) < 0) {
            fprintf(stderr, "libvirt-sandbox-init-qemu: %s: error reading %s: %s\n",
                    __func__, filename, strerror(errno));
            exit_poweroff();
        }
        if (got == 0)
            break;

        offset += got;
    }
    *len = offset;
    close(fd);
    return data;
}


static int
has_suffix(const char *filename, const char *ext)
{
    char *offset = strstr(filename, ext);
    return  (offset &&
             offset[strlen(ext)] == '\0');
}

#if WITH_LZMA
static char *
load_module_file_lzma(const char *filename, size_t *len)
{
    lzma_stream st = LZMA_STREAM_INIT;
    char *xzdata;
    size_t xzlen;
    char *data;
    lzma_ret ret;

    *len = 0;

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, filename);

    if ((ret = lzma_stream_decoder(&st, UINT64_MAX,
                                   LZMA_CONCATENATED)) != LZMA_OK) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s: lzma init failure: %d\n",
                __func__, filename, ret);
        exit_poweroff();
    }
    xzdata = readall(filename, &xzlen);

    st.next_in = (unsigned char *)xzdata;
    st.avail_in = xzlen;

    *len = 32 * 1024;
    if (!(data = malloc(*len))) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, strerror(errno));
        exit_poweroff();
    }

    st.next_out = (unsigned char *)data;
    st.avail_out = *len;

    do {
        ret = lzma_code(&st, LZMA_FINISH);
        if (st.avail_out == 0) {
            size_t used = *len;
            *len += (32 * 1024);
            if (!(data = realloc(data, *len))) {
                fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, strerror(errno));
                exit_poweroff();
            }
            st.next_out = (unsigned char *)data + used;
            st.avail_out = *len - used;
        }
        if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
            fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s: lzma decode failure: %d\n",
                    __func__, filename, ret);
            exit_poweroff();
        }
    } while (ret != LZMA_STREAM_END);
    lzma_end(&st);
    free(xzdata);
    return data;
}
#else
static char *
load_module_file_lzma(const char *filename, size_t *len)
{
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: "
            "lzma support disabled, can't read module %s\n", __func__, filename);
    exit_poweroff();
}
#endif /* WITH_LZMA */

#if WITH_ZLIB
static char *
load_module_file_zlib(const char *filename, size_t *len)
{
    gzFile fp;
    char *data;
    unsigned int avail;
    size_t total;
    int got;

    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, filename);

    if (!(fp = gzopen(filename, "rb"))) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s: gzopen failure\n",
                __func__, filename);
        exit_poweroff();
    }

    *len = 32 * 1024;
    if (!(data = malloc(*len))) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, strerror(errno));
        exit_poweroff();
    }

    avail = *len;
    total = 0;

    do {
        got = gzread(fp, data + total, avail);

        if (got < 0) {
            fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s: gzread failure\n",
                __func__, filename);
            exit_poweroff();
        }

        total += got;
        if (total >= *len) {
            *len += (32 * 1024);
            if (!(data = realloc(data, *len))) {
                fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, strerror(errno));
                exit_poweroff();
            }
            avail = *len - total;
        }
    } while (got > 0);

    gzclose(fp);
    return data;
}
#else
static char *
load_module_file_zlib(const char *filename, size_t *len)
{
    fprintf(stderr, "libvirt-sandbox-init-qemu: %s: "
            "zlib support disabled, can't read module %s\n", __func__, filename);
    exit_poweroff();
}
#endif /* WITH_ZLIB */

static char *
load_module_file_raw(const char *filename, size_t *len)
{
    if (debug)
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: %s\n", __func__, filename);
    return readall(filename, len);
}

static char *
load_module_file(const char *filename, size_t *len)
{
    if (has_suffix(filename, ".ko.xz"))
        return load_module_file_lzma(filename, len);
    else if (has_suffix(filename, ".ko.gz"))
        return load_module_file_zlib(filename, len);
    else
        return load_module_file_raw(filename, len);
}


static void
insmod(const char *filename)
{
    char *data;
    size_t len;

    data = load_module_file(filename, &len);

    if (init_module(data, (unsigned long)len, "") < 0) {
        const char *msg;
        switch (errno) {
        case ENOEXEC:
            msg = "Invalid module format";
            break;
        case ENOENT:
            msg = "Unknown symbol in module";
            break;
        case ESRCH:
            msg = "Module has wrong symbol version";
            break;
        case EINVAL:
            msg = "Invalid parameters";
            break;
        default:
            msg = strerror(errno);
        }
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: error loading %s: %s\n",
                __func__, filename, msg);
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


static void set_debug(void)
{
    if (mkdir("/proc", 0755) < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mkdir /proc: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }
    if (mount("none", "/proc", "proc", 0, "") < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot mount /proc: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }
    FILE *fp = fopen("/proc/cmdline", "r");

    if (fp && fgets(line, sizeof line, fp)) {
        if (strstr(line, "debug"))
            debug=1;
    }
    if (fp)
        fclose(fp);
    if (umount("/proc") < 0) {
        fprintf(stderr, "libvirt-sandbox-init-qemu: %s: cannot unmount /proc: %s\n",
                __func__, strerror(errno));
        exit_poweroff();
    }
}

static int
has_command_arg(const char *name,
                char **val)
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
        return -1;

    start += strlen(name);
    if (start[0] == '\n' ||
        start[0] == ' ') {
        *val = NULL;
        return 0;
    }
    end = strstr(start, " ");
    if (!end)
        end = strstr(start, "\n");
    if (end)
        *val = strndup(start, end-start);
    else
        *val = strdup(start);
    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
