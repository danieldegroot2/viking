/*
 * viking -- GPS Data and Topo Analyzer, Explorer, and Manager
 *
 * Copyright (C) 2003-2005, Evan Battaglia <gtoevan@gmx.net>
 * Copyright (C) 2012, Guilhem Bonnefille <guilhem.bonnefille@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _VIKING_FILE_H
#define _VIKING_FILE_H

#include <stdio.h>
#include <glib.h>

#include "vikaggregatelayer.h"
#include "viktrwlayer.h"
#include "vikviewport.h"

G_BEGIN_DECLS

typedef enum {
FILE_TYPE_GPSPOINT=1,
FILE_TYPE_GPSMAPPER=2,
FILE_TYPE_GPX=3,
FILE_TYPE_KML=4,
FILE_TYPE_GEOJSON=5,
FILE_TYPE_TCX=6,
} VikFileType_t;

gboolean a_file_check_ext ( const gchar *filename, const gchar *fileext );

/*
 * Function to determine if a filename is a 'viking' type file
 */
gboolean check_file_magic_vik ( const gchar *filename );

typedef enum {
  LOAD_TYPE_READ_FAILURE,
  LOAD_TYPE_GPSBABEL_FAILURE,
  LOAD_TYPE_GPX_FAILURE,
  LOAD_TYPE_TCX_FAILURE,
  LOAD_TYPE_KML_FAILURE,
  LOAD_TYPE_FIT_FAILURE,
  LOAD_TYPE_UNSUPPORTED_FAILURE,
  LOAD_TYPE_OTHER_FAILURE_NON_FATAL,
  LOAD_TYPE_VIK_FAILURE_NON_FATAL,
  LOAD_TYPE_VIK_SUCCESS,
  LOAD_TYPE_OTHER_SUCCESS,
} VikLoadType_t;

gchar *append_file_ext ( const gchar *filename, VikFileType_t type );

VikLoadType_t a_file_load_stream ( FILE *f,
                                   const gchar *filename,
                                   VikAggregateLayer *top,
                                   VikViewport *vp,
                                   VikTrwLayer *vtl,
                                   gboolean new_layer,
                                   gboolean external,
                                   const gchar *dirpath,
                                   const gchar *name );

VikLoadType_t a_file_load ( VikAggregateLayer *top,
                            VikViewport *vp,
                            VikTrwLayer *vtl,
                            const gchar *filename_or_uri,
                            gboolean new_layer,
                            gboolean external,
                            const gchar *name );

gboolean a_file_save ( VikAggregateLayer *top, gpointer vp, const gchar *filename );
/* Only need to define VikTrack if the file type is FILE_TYPE_GPX_TRACK */
gboolean a_file_export ( VikTrwLayer *vtl, const gchar *filename, VikFileType_t file_type, VikTrack *trk, gboolean write_hidden );
gboolean a_file_export_babel ( VikTrwLayer *vtl, const gchar *filename, const gchar *format,
                               gboolean tracks, gboolean routes, gboolean waypoints, const gchar *suboptions );

void file_write_layer_param ( FILE *f, const gchar *name, VikLayerParamType type, VikLayerParamData data );

G_END_DECLS

#endif
