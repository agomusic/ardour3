/* libgnomecanvas/gnome-canvas-simplerect.h: GnomeCanvas item for simple rects
 *
 * Copyright (C) 2001 Paul Davis <pbd@op.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GNOME_CANVAS_SIMPLERECT_H__
#define __GNOME_CANVAS_SIMPLERECT_H__

#include <stdint.h>

#include <libgnomecanvas/libgnomecanvas.h>

G_BEGIN_DECLS

/* Wave viewer item for canvas.
 */

#define GNOME_TYPE_CANVAS_SIMPLERECT            (gnome_canvas_simplerect_get_type ())
#define GNOME_CANVAS_SIMPLERECT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_SIMPLERECT, GnomeCanvasSimpleRect))
#define GNOME_CANVAS_SIMPLERECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_SIMPLERECT, GnomeCanvasSimpleRectClass))
#define GNOME_IS_CANVAS_SIMPLERECT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_SIMPLERECT))
#define GNOME_IS_CANVAS_SIMPLERECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_SIMPLERECT))
#define GNOME_CANVAS_SIMPLERECT_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CANVAS_SIMPLERECT, GnomeCanvasSimpleRectClass))

typedef struct _GnomeCanvasSimpleRect            GnomeCanvasSimpleRect;
typedef struct _GnomeCanvasSimpleRectClass       GnomeCanvasSimpleRectClass;

struct _GnomeCanvasSimpleRect
{
    GnomeCanvasItem item;
    double x1, y1, x2, y2;
    gboolean     fill;
    gboolean     draw;
    gboolean     full_draw_on_update;
    uint32_t fill_color;
    uint32_t outline_color;
    uint32_t outline_pixels;

    /* cached values set during update/used during render */

    unsigned char fill_r, fill_b, fill_g, fill_a;
    unsigned char outline_r, outline_b, outline_g;
    unsigned char outline_what;
    gint32 bbox_ulx, bbox_uly;
    gint32 bbox_lrx, bbox_lry;
};

struct _GnomeCanvasSimpleRectClass {
	GnomeCanvasItemClass parent_class;
};

GType gnome_canvas_simplerect_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GNOME_CANVAS_SIMPLERECT_H__ */
