/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright © 2024 Noé Lopez <noelopez@free.fr>
 *
 * Licensed under the GNU General Public License Version 2
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
 * along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "pk-guile-interface.h"
#include "libguile/foreign.h"
#include "libguile/list.h"
#include "libguile/load.h"
#include "libguile/scm.h"
#include "libguile/strings.h"
#include "pk-backend-job.h"
#include "pk-backend.h"
#include <stdio.h>
#include <pwd.h>
#include <sys/syscall.h>

static const char MODULE_NAME[] = "packagekit pk-guile-interface";

static SCM scm_pk_search;
static SCM scm_pk_get_details;
static SCM scm_pk_resolve;
static SCM scm_pk_install;
static SCM scm_pk_remove;
static SCM scm_pk_upgrade;

/* Downgrade our permissions to avoid running arbitrary guile code as
 * root (since we load the user’s guix).
 *
 * We are using syscalls since the libc function will change the ids
 * for all threads. */
static void
set_thread_permissions (uid_t uid, gid_t gid)
{
	//TODO: drop ambient capabilities
	syscall(SYS_setresuid, uid, uid, uid); /* TODO: abort in case of errors in these */
	syscall(SYS_setresgid, gid, gid, gid);
}

static char *
get_user_profiles_path (const char *user_name)
{
	gchar *profiles;
	gchar *ret;

	profiles = g_strjoin("/", "/var/guix/profiles/per-user", user_name, NULL);
	ret = realpath(profiles, NULL);
	g_free(profiles);
	return ret;
}

static void
setup_environment (const char *profiles_path)
{
	g_autofree gchar *guix_scm = g_strconcat(profiles_path, "/current-guix/share/guile/site/3.0", NULL);
	g_autofree gchar *guix_go = g_strconcat(profiles_path, "/current-guix/lib/guile/3.0/site-ccache", NULL);
	g_autofree char *scm_path = g_strjoin(":", guix_scm, GUILE_LOAD_PATH, NULL);
	g_autofree char *go_path = g_strjoin(":", guix_go, GUILE_LOAD_COMPILED_PATH, NULL);

	setenv("GUILE_LOAD_PATH", scm_path, 1);
	setenv("GUILE_LOAD_COMPILED_PATH", go_path, 1);
}

// TODO : error handling?
static void
setup_guile (const char *profiles_path)
{
	g_autofree gchar *progname = g_strconcat(profiles_path, "/current-guix/bin/guix", NULL);
	char *argv[] = { progname };

	scm_set_program_arguments (1, argv, NULL);
	scm_primitive_load_path (scm_from_locale_string ("packagekit/pk-guile-interface"));
	scm_pk_search = scm_c_public_lookup (MODULE_NAME, "pk-search");
	scm_pk_get_details = scm_c_public_lookup (MODULE_NAME, "pk-get-details");
	scm_pk_resolve = scm_c_public_lookup (MODULE_NAME, "pk-resolve");
	scm_pk_install = scm_c_public_lookup (MODULE_NAME, "pk-install");
	scm_pk_remove = scm_c_public_lookup (MODULE_NAME, "pk-remove");
	scm_pk_upgrade = scm_c_public_lookup (MODULE_NAME, "pk-upgrade");
}

/* Calls P inside a guile environment with the user’s guix and running
 * as the user. */
void
call_with_guile (PkBackendJob* job, GVariant* params, void (*p)(const struct guix_job_data *))
{
	guint uid = pk_backend_job_get_uid (job);
	struct passwd* user = getpwuid (uid);
	g_autofree char *profiles_path = get_user_profiles_path(user->pw_name);
	struct guix_job_data data = { job, params, profiles_path };

	pk_backend_job_set_status(job, PK_STATUS_ENUM_RUNNING);
	set_thread_permissions (uid, user->pw_gid);
	setup_environment (profiles_path);
	scm_init_guile ();
	setup_guile (profiles_path);
	p(&data); /* TODO: check if this works with multiple users doing requests */
	pk_backend_job_finished(job);
}


static SCM scm_pop (SCM *list)
{
	SCM head = SCM_CAR (*list);
	*list = SCM_CDR (*list);
	return head;
}

static SCM
args_to_scm_list (const gchar **args)
{
	if (*args == NULL)
		return SCM_EOL;
	return scm_cons(scm_from_locale_string(*args), args_to_scm_list (args + 1));
}

static SCM
alist_cons_filter(SCM alist, const char *filter, gboolean negated)
{
	return scm_cons (
		scm_cons(
			scm_from_utf8_symbol (filter),
			scm_from_bool (negated)),
		alist);
}

static SCM
filters_to_scm_alist (const PkBitfield filters)
{
	SCM alist = SCM_EOL;
	if (pk_bitfield_contain (filters, PK_FILTER_ENUM_INSTALLED))
		alist = alist_cons_filter (alist, "installed", TRUE);
	if (pk_bitfield_contain (filters, PK_FILTER_ENUM_NOT_INSTALLED))
		alist = alist_cons_filter (alist, "installed", FALSE);
	if (pk_bitfield_contain (filters, PK_FILTER_ENUM_ARCH))
		alist = alist_cons_filter (alist, "arch", FALSE);
	if (pk_bitfield_contain (filters, PK_FILTER_ENUM_NOT_ARCH))
		alist = alist_cons_filter (alist, "arch", TRUE);
	return alist;
}

static void
submit_package_list (PkBackendJob *job, SCM packages)
{
	SCM package;
	PkInfoEnum info = PK_INFO_ENUM_AVAILABLE;
	const gchar *package_id;
	const gchar *package_description;

	if (scm_is_true (scm_null_p(packages)))
		return;
	package = scm_car (packages);
	package_id = scm_to_locale_string (scm_car (package));
	package_description = scm_to_locale_string (scm_cdr (package));
	pk_backend_job_package (job, info, package_id, package_description);
	submit_package_list (job, scm_cdr (packages));
}

static void
submit_package_list_details (PkBackendJob *job, SCM packages)
{
	SCM package;
	const gchar *package_id;
	const gchar *package_description;
	const gchar *package_synopsis;
	const gchar *package_license;
	const gchar *package_homepage;

	if (scm_is_true (scm_null_p(packages)))
		return;
	package = scm_car (packages);
	package_id = scm_to_locale_string (scm_pop (&package));
	package_description = scm_to_locale_string (scm_pop (&package));
	package_synopsis = scm_to_locale_string (scm_pop (&package));
	package_license = scm_to_locale_string (scm_pop (&package));
	package_homepage = scm_to_locale_string (scm_pop (&package));
	pk_backend_job_details(job, package_id, package_synopsis, package_license, PK_GROUP_ENUM_UNKNOWN, package_description, package_homepage, 0);
	submit_package_list (job, scm_cdr (packages));
}

/* Gets the search and filters from a job’s parameters and turns them
 * into scheme lists written to PACKAGES and FILTERS that can be given
 * to the guile interface */
static gboolean
search_and_filters_to_scm (const struct guix_job_data *data, SCM *scm_packages, SCM *scm_filters)
{
	const gchar **search;
	PkBitfield filters;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(t^a&s)", &filters, &search);
	if (search == NULL) {
		pk_backend_job_error_code (job, PK_ERROR_ENUM_PACKAGE_ID_INVALID, "No search provided");
		return FALSE;
	}
	*scm_packages = args_to_scm_list (search);
	*scm_filters = filters_to_scm_alist (filters);
	return TRUE;
}

/* Gets the search from a job’s parameters and turns it into a
 * scheme list written to PACKAGES that can be given to the guile
 * interface */
static gboolean
search_to_scm (const struct guix_job_data *data, SCM *packages)
{
	const gchar **search;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(^a&s)", &search);
	if (search == NULL) {
		pk_backend_job_error_code (job, PK_ERROR_ENUM_PACKAGE_ID_INVALID, "No search provided");
		return FALSE;
	}
	*packages = args_to_scm_list (search);
	return TRUE;
}


// Functions to be used as arguments to call_with_guile.

void
guix_search (const struct guix_job_data *data)
{
	SCM result;
	SCM search;
	SCM filters;

	if (!search_and_filters_to_scm (data, &search, &filters))
		return;
	pk_backend_job_set_status(data->job, PK_STATUS_ENUM_QUERY);
	result = scm_call_2(scm_variable_ref (scm_pk_search), search, filters);
	submit_package_list (data->job, result);
}

void
guix_resolve (const struct guix_job_data *data)
{
	SCM result;
	SCM search;
	SCM filters;

	if (!search_and_filters_to_scm (data, &search, &filters))
		return;
	pk_backend_job_set_status(data->job, PK_STATUS_ENUM_QUERY);
	result = scm_call_2(scm_variable_ref (scm_pk_resolve), search, filters);
	submit_package_list (data->job, result);
}

void
guix_details (const struct guix_job_data *data)
{
	SCM result;
	SCM search;

	if (!search_to_scm (data, &search))
		return;
	result = scm_call_1(scm_variable_ref (scm_pk_get_details), search);
	submit_package_list_details (data->job, result);
}

static void
guix_package (const struct guix_job_data *data, SCM action)
{
	SCM packages;
	SCM filters;

	if (!search_and_filters_to_scm (data, &packages, &filters))
		return;
	scm_call_1 (scm_variable_ref(action), packages);
	pk_backend_job_finished(data->job);
}

void
guix_install (const struct guix_job_data *data)
{
	return guix_package (data, scm_pk_install);
}

void
guix_remove (const struct guix_job_data *data)
{
	return guix_package (data, scm_pk_remove);
}

void
guix_upgrade (const struct guix_job_data *data)
{
	return guix_package (data, scm_pk_upgrade);
}
