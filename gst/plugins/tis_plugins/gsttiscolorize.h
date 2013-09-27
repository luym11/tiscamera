/* GStreamer
 * Copyright (C) 2013 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_TISCOLORIZE_H_
#define _GST_TISCOLORIZE_H_

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_TISCOLORIZE   (gst_tiscolorize_get_type())
#define GST_TISCOLORIZE(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TISCOLORIZE,GstTisColorize))
#define GST_TISCOLORIZE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TISCOLORIZE,GstTisColorizeClass))
#define GST_IS_TISCOLORIZE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TISCOLORIZE))
#define GST_IS_TISCOLORIZE_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TISCOLORIZE))

typedef struct _GstTisColorize GstTisColorize;
typedef struct _GstTisColorizeClass GstTisColorizeClass;

struct _GstTisColorize
{
	GstBaseTransform base_tiscolorize;

	GstPad *sinkpad;
	GstPad *srcpad;
	
	gint gain_red;
	gint gain_green;
	gint gain_blue;

	gboolean auto_wb;
	
};

struct _GstTisColorizeClass
{
  GstBaseTransformClass base_tiscolorize_class;
};

GType gst_tiscolorize_get_type (void);

G_END_DECLS

#endif
