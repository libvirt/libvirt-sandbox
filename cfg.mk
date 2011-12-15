# Customize Makefile.maint.                           -*- makefile -*-
# Copyright (C) 2008-2011 Red Hat, Inc.
# Copyright (C) 2003-2008 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Tests not to run as part of "make distcheck".
local-checks-to-skip =			\
  changelog-check			\
  check-AUTHORS				\
  makefile-check			\
  makefile_path_separator_check		\
  patch-check				\
  sc_GPL_version			\
  sc_always_defined_macros		\
  sc_cast_of_alloca_return_value	\
  sc_cross_check_PATH_usage_in_tests	\
  sc_dd_max_sym_length			\
  sc_error_exit_success			\
  sc_file_system			\
  sc_immutable_NEWS			\
  sc_makefile_path_separator_check	\
  sc_obsolete_symbols			\
  sc_prohibit_always_true_header_tests  \
  sc_prohibit_S_IS_definition		\
  sc_prohibit_atoi_atof			\
  sc_prohibit_hash_without_use		\
  sc_prohibit_jm_in_m4			\
  sc_prohibit_quote_without_use		\
  sc_prohibit_quotearg_without_use	\
  sc_prohibit_stat_st_blocks		\
  sc_root_tests				\
  sc_space_tab				\
  sc_sun_os_names			\
  sc_system_h_headers			\
  sc_texinfo_acronym			\
  sc_tight_scope			\
  sc_two_space_separator_in_usage	\
  sc_error_message_uppercase		\
  sc_program_name			\
  sc_require_test_exit_idiom		\
  sc_makefile_check			\
  sc_useless_cpp_parens

# Files that should never cause syntax check failures.
VC_LIST_ALWAYS_EXCLUDE_REGEX = \
  (^HACKING|\.po|maint.mk)$$

# Functions like free() that are no-ops on NULL arguments.
useless_free_options =				\
  --name=g_free

# Ensure that no C source file, docs, or rng schema uses TABs for
# indentation.  Also match *.h.in files, to get libvirt.h.in.  Exclude
# files in gnulib, since they're imported.
space_indent_files=(\.(rng|s?[ch](\.in)?|html.in|py)|(daemon|tools)/.*\.in)
sc_TAB_in_indentation:
	@prohibit='^ *	'						\
	in_vc_files='$(space_indent_files)$$'				\
	halt='indent with space, not TAB, in C, sh, html, py, and RNG schemas' \
	  $(_sc_search_regexp)

# G_GNUC_UNUSED should only be applied in implementations, not
# header declarations
sc_avoid_attribute_unused_in_header:
	@prohibit='^[^#]*G_GNUC_UNUSED([^:]|$$)'			\
	in_vc_files='\.h$$'						\
	halt='use G_GNUC_UNUSED in .c rather than .h files'		\
	  $(_sc_search_regexp)

# Enforce recommended preprocessor indentation style.
sc_preprocessor_indentation:
	@if cppi --version >/dev/null 2>&1; then			\
	  $(VC_LIST_EXCEPT) | grep '\.[ch]$$' | xargs cppi -a -c	\
	    || { echo '$(ME): incorrect preprocessor indentation' 1>&2;	\
		exit 1; };						\
	else								\
	  echo '$(ME): skipping test $@: cppi not installed' 1>&2;	\
	fi

sc_copyright_format:
	@require='Copyright .*Red 'Hat', Inc\.'				\
	containing='Copyright .*Red 'Hat				\
	halt='Red Hat copyright is missing Inc.'			\
	  $(_sc_search_regexp)
	@prohibit='Copyright [^(].*Red 'Hat				\
	halt='consistently use (C) in Red Hat copyright'		\
	  $(_sc_search_regexp)
	@prohibit='\<Red''Hat\>'					\
	halt='spell Red Hat as two words'				\
	  $(_sc_search_regexp)

# We don't use this feature of maint.mk.
prev_version_file = /dev/null

# Give credit where due:
# Ensure that each commit author email address (possibly mapped via
# git log's .mailmap) appears in our AUTHORS file.
sc_check_author_list:
	@fail=0;							\
	for i in $$(git log --pretty=format:%aE%n|sort -u|grep -v '^$$'); do \
	  sanitized=$$(echo "$$i"|LC_ALL=C sed 's/\([^a-zA-Z0-9_@-]\)/\\\1/g'); \
	  grep -iq "<$$sanitized>" $(srcdir)/AUTHORS			\
	    || { printf '%s\n' "$$i" >&2; fail=1; };			\
	done;								\
	test $$fail = 1							\
	  && echo '$(ME): committer(s) not listed in AUTHORS' >&2;	\
	test $$fail = 0


exclude_file_name_regexp--sc_bindtextdomain = ^(libvirt-sandbox/tests)|(libvirt-sandbox/libvirt-sandbox-init-*)|(bin/virt-sandbox.c)

exclude_file_name_regexp--sc_preprocessor_indentation = ^*/*.[ch]

exclude_file_name_regexp--sc_prohibit_strcmp = ^libvirt-sandbox/libvirt-sandbox-init-qemu.c
