/*
 * JULEA - Flexible storage framework
 * Copyright (C) 2021-2023 Michael Kuhn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 **/

#ifndef JULEA_CONFIGURATION_INTERNAL_H
#define JULEA_CONFIGURATION_INTERNAL_H

#if !defined(JULEA_H) && !defined(JULEA_COMPILATION)
#error "Only <julea.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS
/**
 * \addtogroup JConfiguration Configuration
 *
 * @{
 **/

/**
 * Initializes the configuration.
 *
 * \private
 */
G_GNUC_INTERNAL void j_configuration_init(void);

/**
 * Shuts down the configuration.
 *
 * \private
 */
G_GNUC_INTERNAL void j_configuration_fini(void);

/**
 * @}
 **/

G_END_DECLS

#endif
