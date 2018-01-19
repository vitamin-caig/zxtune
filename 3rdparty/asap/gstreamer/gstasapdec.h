/*
 * gstasapdec.c - ASAP plugin for GStreamer
 *
 * Copyright (C) 2011  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GST_ASAPDEC_H__
#define __GST_ASAPDEC_H__

#include <gst/gst.h>
#include "asap.h"

G_BEGIN_DECLS

#define GST_TYPE_ASAPDEC \
  (gst_asap_dec_get_type())
#define GST_ASAPDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ASAPDEC,GstAsapDec))
#define GST_ASAPDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ASAPDEC,GstAsapDecClass))
#define GST_IS_ASAPDEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ASAPDEC))
#define GST_IS_ASAPDEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ASAPDEC))

typedef struct _GstAsapDec      GstAsapDec;
typedef struct _GstAsapDecClass GstAsapDecClass;

struct _GstAsapDec
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  ASAP *asap;
  gboolean playing;
  gint duration;
  gint tune_number;
  gint module_len;
  guchar module[ASAPInfo_MAX_MODULE_LENGTH];
};

struct _GstAsapDecClass
{
  GstElementClass parent_class;
};

GType gst_asap_dec_get_type (void);

G_END_DECLS

#endif /* __GST_ASAPDEC_H__ */
