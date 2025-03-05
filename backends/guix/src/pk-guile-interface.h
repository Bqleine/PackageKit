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

#ifndef PK_GUILE_INTERFACE_H
#define PK_GUILE_INTERFACE_H

#include "pk-backend.h"
#include <libguile.h>

struct guix_job_data {
	PkBackendJob *job;
	GVariant *params;
	const char *profiles;
};

void call_with_guile (PkBackendJob* job, GVariant* params, void *p);

void guix_search (struct guix_job_data *data);
void guix_resolve (struct guix_job_data *data);
void guix_install (struct guix_job_data *data);
void guix_details (struct guix_job_data *data);
#endif
