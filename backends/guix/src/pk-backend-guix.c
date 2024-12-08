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

#include "pk-backend.h"
#include "pk-backend-job.h"
#include "pk-guile-interface.h"

const gchar *
pk_backend_get_description (PkBackend *backend)
{
	return "Guix – the reproducible and functional package manager";
}

const gchar *
pk_backend_get_author (PkBackend *backend)
{
	return "Noé Lopez <noelopez@free.fr>";
}

PkBitfield
pk_backend_get_groups (PkBackend *backend)
{
	return 0;
}

PkBitfield
pk_backend_get_filters (PkBackend *backend)
{
	return pk_bitfield_from_enums (
		PK_FILTER_ENUM_SUPPORTED,
		PK_FILTER_ENUM_NOT_SUPPORTED,
		PK_FILTER_ENUM_INSTALLED,
		PK_FILTER_ENUM_NOT_INSTALLED,
		-1
	);
}

void
pk_backend_initialize (GKeyFile *conf, PkBackend *packend)
{}

void
pk_backend_destroy (PkBackend *backend)
{}

void
pk_backend_get_details (PkBackend *backend,
			PkBackendJob *job,
			gchar **package_ids)
{
	pk_backend_job_details (job,
				package_ids[0],
				"Hello, GNU world: Un exemple de paquet GNU",
				"gpl3+",
				PK_GROUP_ENUM_UNKNOWN,
				"GNU Hello affiche le message \"Hello, world!\" puis se termine.  Il"
"sert d'exemple pour les pratiques de programmation GNU standard.  Il supporte"
"les arguments en ligne de commande, plusieurs langages, etc.",
				"https://www.gnu.org/software/hello/",
				0);
	pk_backend_job_finished(job);
}

void pk_backend_get_updates (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters)
{
	pk_backend_job_package (job, PK_INFO_ENUM_NORMAL, pk_package_id_build("hello", "3.5", "x86_64", "guix"), "Hey, I am a package");
	pk_backend_job_finished(job);
}

void
pk_backend_refresh_cache (PkBackend *backend,
			PkBackendJob *job,
			gboolean force)
{
	if (!force) {
		pk_backend_job_finished(job);
		return;
	}
	pk_backend_job_finished(job);
}

void pk_backend_resolve (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters,
			gchar **packages)
{
	pk_backend_job_package (job, PK_INFO_ENUM_AVAILABLE, pk_package_id_build("hello", "3.5", "x86_64", "guix"), "Hey, I am a package");
	pk_backend_job_package (job, PK_INFO_ENUM_AVAILABLE, pk_package_id_build("openttd", "14.1", "x86_64", "guix"), "Hey, I am a package");
	pk_backend_job_finished(job);
}

void
pk_backend_search_details (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters,
			gchar **search)
{
	pk_backend_job_thread_create (job, guix_search, NULL, NULL);
}

void
pk_backend_search_files (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters,
			gchar **search)
{
	pk_backend_job_thread_create (job, guix_search, NULL, NULL);
}

void
pk_backend_search_groups (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters,
			gchar **search)
{
	pk_backend_job_thread_create (job, guix_search, NULL, NULL);
}

void
pk_backend_search_names (PkBackend *backend,
			PkBackendJob *job,
			PkBitfield filters,
			gchar **search)
{
	pk_backend_job_thread_create (job, guix_search, NULL, NULL);
}

void
pk_backend_get_packages (PkBackend* backend,
			PkBackendJob* job,
			PkBitfield filters)
{
	pk_backend_job_package (job, PK_INFO_ENUM_AVAILABLE, pk_package_id_build("hello", "3.5", "x86_64", "guix"), "Hey, I am a package");
	pk_backend_job_package (job, PK_INFO_ENUM_AVAILABLE, pk_package_id_build("openttd", "14.1", "x86_64", "guix"), "Hey, I am a package");
	pk_backend_job_finished(job);
}
