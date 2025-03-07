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

/* Downgrade our permissions to avoid running arbitrary guile code as
 * root (since we load the user’s guix).  FIXME: I wonder if this is
 * enough.
 *
 * We are using syscalls since the libc function will change the ids
 * for all threads. */
static void
set_thread_permissions (uid_t uid, gid_t gid)
{
	syscall(SYS_setresuid, uid, uid, uid);
	syscall(SYS_setresgid, gid, gid, gid);
}

static char *
get_user_profiles (PkBackendJob* job)
{
	guint uid = pk_backend_job_get_uid (job);
	struct passwd* uid_ent = getpwuid (uid);
	gchar *profiles;
	gchar *ret;

	set_thread_permissions(uid, uid_ent->pw_gid);
	if (uid_ent == NULL)
		g_error("Failed to get HOME");
	profiles = g_strjoin("/", "/var/guix/profiles/per-user", uid_ent->pw_name, NULL);
	ret = realpath(profiles, NULL);
	g_free(profiles);
	return ret;
}

static void
setup_environment (const char *profiles)
{
	g_autofree gchar *guix_scm = g_strconcat(profiles, "/current-guix/share/guile/site/3.0", NULL);
	g_autofree gchar *guix_go = g_strconcat(profiles, "/current-guix/lib/guile/3.0/site-ccache", NULL);
	g_autofree char *scm_path = g_strjoin(":", guix_scm, GUILE_LOAD_PATH, NULL);
	g_autofree char *go_path = g_strjoin(":", guix_go, GUILE_LOAD_COMPILED_PATH, NULL);

	setenv("GUILE_LOAD_PATH", scm_path, 1);
	setenv("GUILE_LOAD_COMPILED_PATH", go_path, 1);
}

// TODO : error handling?
static void
setup_guile (const char *profiles)
{
	g_autofree gchar *progname = g_strconcat(profiles, "/current-guix/bin/guix", NULL);
	char *argv[] = { progname };

	scm_set_program_arguments (1, argv, NULL);
	scm_primitive_load_path (scm_from_locale_string ("packagekit/pk-guile-interface"));
	scm_pk_search = scm_c_public_lookup (MODULE_NAME, "pk-search");
	scm_pk_get_details = scm_c_public_lookup (MODULE_NAME, "pk-get-details");
	scm_pk_resolve = scm_c_public_lookup(MODULE_NAME, "pk-resolve");
	scm_pk_install = scm_c_public_lookup(MODULE_NAME, "pk-install");
}


void
call_with_guile (PkBackendJob* job, GVariant* params, void *p)
{
	g_autofree char *profiles = get_user_profiles(job);
	struct guix_job_data data = { job, params, profiles };

	setup_environment(data.profiles);
	scm_with_guile (p, &data); /* TODO: check if this works with multiple users doing requests */
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


void
guix_search (struct guix_job_data *data)
{
	SCM result;
	SCM regexs;
	const gchar **search;
	PkBitfield filters;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(t^a&s)", &filters, &search);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_SETUP);
	setup_guile(data->profiles);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_QUERY);
	if (search == NULL)
		return;		/* TODO call job error ? */
	regexs = args_to_scm_list (search);
	result = scm_call_1(scm_variable_ref (scm_pk_search), regexs);
	submit_package_list (job, result);
	pk_backend_job_finished(job);
}

void
guix_resolve (struct guix_job_data *data)
{
	SCM result;
	SCM regexs;
	const gchar **search;
	PkBitfield filters;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(t^a&s)", &filters, &search);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_SETUP);
	setup_guile(data->profiles);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_QUERY);
	if (search == NULL)
		return;		/* TODO call job error ? */
	regexs = args_to_scm_list (search);
	result = scm_call_1(scm_variable_ref (scm_pk_resolve), regexs);
	submit_package_list (job, result);
	pk_backend_job_finished(job);
}

void
guix_details (struct guix_job_data *data)
{
	SCM result;
	SCM regexs;
	const gchar **search;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(^a&s)", &search);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_SETUP);
	setup_guile(data->profiles);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_QUERY);
	if (search == NULL)
		return;		/* TODO call job error ? */
	regexs = args_to_scm_list (search);
	result = scm_call_1(scm_variable_ref (scm_pk_get_details), regexs);
	submit_package_list_details (job, result);
	pk_backend_job_finished(job);
}

void
guix_install (struct guix_job_data *data)
{
	PkBitfield filters;
	const gchar **package_ids;
	PkBackendJob *job = data->job;
	SCM package_list;

	g_variant_get (data->params, "(t^a&s)", &filters, &package_ids);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_SETUP);
	setup_guile(data->profiles);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_INSTALL);
	if (package_ids == NULL)
		return;		/* TODO error */
	package_list = args_to_scm_list (package_ids);
	scm_call_1 (scm_variable_ref(scm_pk_install), package_list);
	pk_backend_job_finished(job);
}
