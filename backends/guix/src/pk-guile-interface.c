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
#include "libguile/list.h"
#include "libguile/load.h"
#include "libguile/strings.h"
#include "pk-backend-job.h"
#include <stdio.h>

static const char MODULE_NAME[] = "packagekit pk-guile-interface";

static SCM scm_pk_search;

struct search_data {
	PkBackendJob *job;
	GVariant *params;
};

// TODO : find guix profile in path
// TODO : error handling?
static void
setup_guile (void)
{
	char progname[] = "/gnu/store/iygxiv409gqdfz6jj7i9jzhmijwqi5dz-guix-d95588242/bin/guix";
	char *argv[] = { progname };

	scm_set_program_arguments (1, argv, NULL);
	scm_primitive_load_path (scm_from_locale_string ("packagekit/pk-guile-interface"));
	scm_pk_search = scm_c_public_lookup (MODULE_NAME, "pk-search");
}

static SCM
construct_list_from_search (const gchar **search, SCM list)
{
	if (*search == NULL)
		return list;
	return scm_cons(scm_from_locale_string(*search), construct_list_from_search (search + 1, list));
}

static void
submit_search_results (PkBackendJob *job, SCM packages)
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
	submit_search_results (job, scm_cdr (packages));
}

static void *
package_search (struct search_data *data)
{
	SCM result;
	SCM regexs;
	const gchar **search;
	PkBitfield filters;
	PkBackendJob *job = data->job;

	g_variant_get (data->params, "(t^a&s)", &filters, &search);
	pk_backend_job_set_status(job, PK_STATUS_ENUM_SETUP);
	setup_guile ();
	pk_backend_job_set_status(job, PK_STATUS_ENUM_QUERY);
	if (search == NULL)
		return NULL;		/* TODO call job error ? */
	regexs = construct_list_from_search (search, SCM_EOL);
	result = scm_call_1(scm_variable_ref (scm_pk_search), regexs);
	submit_search_results (job, result);
	pk_backend_job_finished(job);
	return NULL;
}

void
guix_search (PkBackendJob* job, GVariant* params, gpointer p)
{
	struct search_data data = { job, params };

	pk_backend_job_package (job, PK_INFO_ENUM_AVAILABLE, "hello;3.5;x86_64;available;guix", "Hey, I am a package");
	pk_backend_job_finished(job);
//	scm_with_guile ((void *(*)(void *)) package_search, &data);
}
