/*
 * virt-sandbox.c: libvirt sandbox command
 *
 * Copyright (C) 2011 Red Hat, Inc.
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

#include <config.h>

#include <libvirt-sandbox/libvirt-sandbox.h>
#include <glib/gi18n.h>

static gboolean do_close(GVirSandboxConsole *con G_GNUC_UNUSED,
                         gboolean error G_GNUC_UNUSED,
                         gpointer opaque)
{
    GMainLoop *loop = opaque;
    g_main_loop_quit(loop);
    return FALSE;
}

static gboolean do_exited(GVirSandboxConsole *con G_GNUC_UNUSED,
                          int status,
                          gpointer opaque)
{
    int *ret = opaque;
    *ret = WEXITSTATUS(status);
    return FALSE;
}

static void libvirt_sandbox_version(void)
{
    g_print(_("%s version %s\n"), PACKAGE, VERSION);

    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
    int ret = EXIT_FAILURE;
    GVirConnection *hv = NULL;
    GVirSandboxConfig *cfg = NULL;
    GVirSandboxConfigInteractive *icfg = NULL;
    GVirSandboxContext *ctx = NULL;
    GVirSandboxContextInteractive *ictx = NULL;
    GVirSandboxConsole *log = NULL;
    GVirSandboxConsole *con = NULL;
    GMainLoop *loop = NULL;
    GError *error = NULL;
    gchar *name = NULL;
    gchar **guestBinds = NULL;
    gchar **hostBinds = NULL;
    gchar **hostImages = NULL;
    gchar **includes = NULL;
    gchar *includefile = NULL;
    gchar *uri = NULL;
    gchar *security = NULL;
    gchar **networks = NULL;
    gchar **cmdargs = NULL;
    gboolean verbose = FALSE;
    gboolean debug = FALSE;
    gboolean shell = FALSE;
    gboolean privileged = FALSE;
    GOptionContext *context;
    GOptionEntry options[] = {
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          libvirt_sandbox_version, N_("display version information"), NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
          N_("display verbose information"), NULL },
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
          N_("display debugging information"), NULL },
        { "connect", 'c', 0, G_OPTION_ARG_STRING, &uri,
          N_("connect to hypervisor"), "URI"},
        { "name", 'n', 0, G_OPTION_ARG_STRING, &name,
          N_("name of the sandbox"), "NAME" },
        { "guest-bind", 'b', 0, G_OPTION_ARG_STRING_ARRAY, &guestBinds,
          N_("bind guest locations together"), "DST-GUEST-DIR=SRC-GUEST-DIR" },
        { "host-bind", 'B', 0, G_OPTION_ARG_STRING_ARRAY, &hostBinds,
          N_("pass host locations through to the guest"), "DST-GUEST-DIR=SRC-HOST-DIR" },
        { "host-image", 'm', 0, G_OPTION_ARG_STRING_ARRAY, &hostImages,
          N_("pass host images through to the guest"), "DST-GUEST-DIR=SRC-HOST-FILE" },
        { "include", 'i', 0, G_OPTION_ARG_STRING_ARRAY, &includes,
          N_("file to copy into custom dir"), "GUEST-PATH=HOST-PATH", },
        { "includefile", 'I', 0, G_OPTION_ARG_STRING, &includefile,
          N_("file contain list of files to include"), "FILE" },
        { "network", 'N', 0, G_OPTION_ARG_STRING_ARRAY, &networks,
          N_("setup network interface properties"), "PATH", },
        { "security", 's', 0, G_OPTION_ARG_STRING, &security,
          N_("security properties"), "PATH", },
        { "privileged", 'p', 0, G_OPTION_ARG_NONE, &privileged,
          N_("run the command privileged"), NULL },
        { "shell", 'l', 0, G_OPTION_ARG_NONE, &shell,
          N_("start a shell"), NULL, },
        { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &cmdargs,
          NULL, "COMMAND-PATH [ARGS...]" },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
    const char *help_msg = N_("Run '" PACKAGE " --help' to see a full list of available command line options");

    if (!gvir_sandbox_init_check(&argc, &argv, &error))
        exit(EXIT_FAILURE);

    g_set_application_name(_("Libvirt Sandbox"));

    context = g_option_context_new (_("- Libvirt Sandbox"));
    g_option_context_add_main_entries (context, options, NULL);
    g_option_context_parse (context, &argc, &argv, &error);
    if (error) {
        g_printerr("%s\n%s\n",
                   error->message,
                   gettext(help_msg));
        goto cleanup;
    }

    g_option_context_free(context);

    if (!cmdargs || (g_strv_length(cmdargs) < 1)) {
        g_printerr(_("\nUsage: %s [OPTIONS] COMMAND-PATH [ARGS...]\n\n%s\n\n"), argv[0], help_msg);
        goto cleanup;
    }

    if (debug) {
        setenv("LIBVIRT_LOG_FILTERS", "1:libvirt", 1);
        setenv("LIBVIRT_LOG_OUTPUTS", "stderr", 1);
    } else if (verbose) {
        setenv("LIBVIRT_LOG_FILTERS", "1:libvirt", 1);
        setenv("LIBVIRT_LOG_OUTPUTS", "stderr", 1);
    }

    loop = g_main_loop_new(g_main_context_default(), FALSE);

    hv = gvir_connection_new(uri);
    if (!gvir_connection_open(hv, NULL, &error)) {
        g_printerr(_("Unable to open connection: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }

    icfg = gvir_sandbox_config_interactive_new(name ? name : "sandbox");
    cfg = GVIR_SANDBOX_CONFIG(icfg);
    gvir_sandbox_config_interactive_set_command(icfg, cmdargs);

    if (privileged) {
        gvir_sandbox_config_set_userid(cfg, 0);
        gvir_sandbox_config_set_groupid(cfg, 0);
        gvir_sandbox_config_set_username(cfg, "root");
    }

    if (hostBinds &&
        !gvir_sandbox_config_add_host_bind_mount_strv(cfg, hostBinds, &error)) {
        g_printerr(_("Unable to parse host bind mounts: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (guestBinds &&
        !gvir_sandbox_config_add_guest_bind_mount_strv(cfg, guestBinds, &error)) {
        g_printerr(_("Unable to parse guest bind mounts: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (hostImages &&
        !gvir_sandbox_config_add_host_image_mount_strv(cfg, hostImages, &error)) {
        g_printerr(_("Unable to parse host image mounts: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (includes &&
        !gvir_sandbox_config_add_host_include_strv(cfg, includes, &error)) {
        g_printerr(_("Unable to parse includes: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (includefile &&
        !gvir_sandbox_config_add_host_include_file(cfg, includefile, &error)) {
        g_printerr(_("Unable to parse include file: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (networks &&
        !gvir_sandbox_config_add_network_strv(cfg, networks, &error)) {
        g_printerr(_("Unable to parse networks: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    if (security &&
        !gvir_sandbox_config_set_security_opts(cfg, security, &error)) {
        g_printerr(_("Unable to parse security: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }

    if (shell)
        gvir_sandbox_config_set_shell(cfg, TRUE);

    if (isatty(STDIN_FILENO))
        gvir_sandbox_config_interactive_set_tty(icfg, TRUE);

    ictx = gvir_sandbox_context_interactive_new(hv, icfg);
    ctx = GVIR_SANDBOX_CONTEXT(ictx);

    if (!gvir_sandbox_context_start(ctx, &error)) {
        g_printerr(_("Unable to start sandbox: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }

    if (!(log = gvir_sandbox_context_get_log_console(ctx, &error))) {
        g_printerr(_("Unable to get log console: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    g_signal_connect(log, "closed", (GCallback)do_close, loop);

    if (!(gvir_sandbox_console_attach_stderr(log, &error))) {
        g_printerr(_("Unable to attach sandbox console: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }

    if (!(con = gvir_sandbox_context_interactive_get_app_console(ictx, &error))) {
        g_printerr(_("Unable to get app console: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }
    g_signal_connect(con, "closed", (GCallback)do_close, loop);
    g_signal_connect(con, "exited", (GCallback)do_exited, &ret);

    if (!(gvir_sandbox_console_attach_stdio(con, &error))) {
        g_printerr(_("Unable to attach sandbox console: %s\n"),
                   error && error->message ? error->message : "unknown");
        goto cleanup;
    }

    g_main_loop_run(loop);

cleanup:
    if (error)
        g_error_free(error);
    if (con)
        gvir_sandbox_console_detach(con, NULL);
    if (ctx) {
        gvir_sandbox_context_stop(ctx, NULL);
        g_object_unref(ctx);
    }
    if (cfg)
        g_object_unref(cfg);
    if (loop)
        g_main_loop_unref(loop);
    if (hv)
        g_object_unref(hv);

    return ret;
}

/*
=head1 NAME

virt-sandbox - Run cmd under a virtual machine sandbox

=head1 SYNOPSIS

virt-sandbox [OPTIONS...] COMMAND [CMDARG1 [CMDARG2 [...]]]

virt-sandbox [OPTIONS...] -S

=head1 DESCRIPTION

Run the C<cmd>  application within a tightly confined virtual machine. The
default sandbox domain only allows applications the ability to read and
write stdin, stdout and any other file descriptors handed to it. It is
not allowed to open any other files. The -M option will mount an alternate
homedir and tmpdir to be used by the sandbox.

If directories are specified with -H or -T the directory will have its
security label changed unless a level is specified with -l. If the MLS/MCS
security level is specified, the user is responsible to set the correct
labels.

=head1 OPTIONS

=over 8

=item B<-c URI>, B<--connect=URI>

Set the libvirt connection URI, defaults to qemu:///session if
omitted. Currently only the QEMU and LXC drivers are supported.

=item B<-n NAME>, B<--name=NAME>

Set the unique name for the sandbox. This defaults to B<sandbox>
but this will need to be changed if more than one sandbox is to
be run concurrently. This is used as the name of the libvirt
virtual machine or container.

=item B<-b DST-GUEST-DIR=SRC-GUEST-DIR>, B<--guest-bind DST-GUEST-DIR=SRC-GUEST-DIR>

Binds the guest location B<SRC-GUEST-DIR> to the location B<DST-GUEST-DIR>
such that their contents are indistinguishable. This option may be
repeated multiple times

=item B<-B DST-GUEST-DIR=SRC-HOST-DIR>, B<--guest-bind DST-GUEST-DIR=SRC-HOST-DIR>

Binds the host location B<SRC-HOST-DIR> to the location B<DST-GUEST-DIR>
such that their contents are indistinguishable. If C<SRC-HOST-DIR> is the
empty string, then a temporary (empty) directory is created on the host
before starting the sandbox and deleted afterwards. The C<--include> option
is useful for populating these temporary directories with copies of host
files. This option may be repeated multiple times.

=item B<-m DST-GUEST-DIR=SRC-HOST-FILE>, B<--host-image DST-GUEST-DIR=SRC-HOST-FILE>

Treats the host file B<SRC-HOST-FILE> as a virtual disk image and mounts it in
the guest at B<DST-GUEST-DIR>. The disk image should be in raw format. This option
may be repeated multiple times.

=item B<-i HOST-PATH>, B<--include=HOST-PATH>

Copy this file from the host into the same location in the guest.
This is used to populate content for mount overrides. This may
be repeated multiple times.

=item B<-I HOST-PATH>, B<--includefile=HOST-PATH>

Copy all files listed in inputfile into the
appropriate temporary sandbox directories.

=item B<-N NETWORK-OPTIONS>, B<--network NETWORK-OPTIONS>

Add a network interface to the sandbox. NETWORK-OPTIONS is a set of
key=val pairs, separated by commas. The following options are valid

=over 4

=item dhcp

Configure the network interface using dhcp. This key takes no value.
No other keys may be specified.

=item address=IP-ADDRESS/PREFIX%BROADCAST

Configure the network interface with the static IPv4 or IPv6 address
B<IP-ADDRESS>. The B<PREFIX> value is the length of the network
prefix in B<IP-ADDRESS>. The optional B<BROADCAST> parameter
specifies the broadcast address. Some examples

  address=192.168.122.1/24
  address=192.168.122.1/24%192.168.122.255
  address=2001:212::204.2/64

=item route=IP-NETWORK/PREFIX%GATEWAY

Configure the network interface with the static IPv4 or IPv6 route
B<IP-NETWORK>. The B<PREFIX> value is the length of the network
prefix in B<IP-NETWORK>. The optional B<GATEWAY> parameter
specifies the address of the gateway. Some examples

  route=192.168.122.255/24%192.168.1.1

=back

=item B<-s SECURITY-OPTIONS>, B<--security=SECURITY-OPTIONS>

Use alternative security options. SECURITY-OPTIONS is a set of key=val pairs,
separated by commas. The following options are valid for SELinux

=over 4

=item type=TYPE

The SELinux security type, defaults to sandbox_t

=item level=LEVEL

The SELinux MCS level, defaults to a randomly allocated level

=back

=item B<-p>, B<--privileged>

Retain root privileges inside the sandbox, rather than dropping privileges
to match the current user identity.

=item B<-l>, B<--shell>

Launch an interactive shell on a secondary console device

=item B<-V>, B<--version>

Display the version number and exit

=item B<-v>, B<--verbose>

Display verbose progress information

=item B<-d>, B<--debug>

Display debugging information

=item B<-h>, B<--help>

Display help information

=back

=head1 EXAMPLES

Run an interactive shell under LXC, replace $HOME with the contents
of $HOME/scratch

  # mkdir $HOME/scratch
  # virt-sandbox -c lxc:/// --host-bind $HOME=$HOME/scratch /bin/sh

Convert an OGG file to WAV inside QEMU

  # virt-sandbox -c qemu:///session  -- /usr/bin/oggdec -Q -o - - < somefile.ogg > somefile.wav

=head1 SEE ALSO

C<sandbox(8)>, C<virsh(1)>

=head1 AUTHORS

Daniel P. Berrange <dan@berrange.com>

=head1 COPYRIGHT

Copyright (C) 2011 Daniel P. Berrange <dan@berrange.com>
Copyright (C) 2011-2012 Red Hat, Inc.

=head1 LICENSE

virt-sandbox is distributed under the terms of the GNU LGPL v2+.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE

=cut

 */
