/* Image item type for GtkCanvas widget
 *
 * GtkCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */
 
 
#include <string.h> /* for memcpy() */
#include <math.h>
#include <stdio.h>
#include "libart_lgpl/art_misc.h"
#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_pixbuf.h"
#include "libart_lgpl/art_rgb_pixbuf_affine.h"
#include "canvas-imageframe.h"
#include <gtk-canvas/gtk-canvas-util.h>
#include <gtk-canvas/gtk-canvastypebuiltins.h>


enum {
	ARG_0,
	ARG_PIXBUF,
	ARG_X,
	ARG_Y,
	ARG_WIDTH,
	ARG_HEIGHT,
	ARG_DRAWWIDTH,
	ARG_ANCHOR
};


static void gtk_canvas_imageframe_class_init(GtkCanvasImageFrameClass* class) ;
static void gtk_canvas_imageframe_init(GtkCanvasImageFrame* image) ;
static void gtk_canvas_imageframe_destroy(GtkObject* object) ;
static void gtk_canvas_imageframe_set_arg(GtkObject* object, GtkArg* arg, guint arg_id) ;
static void gtk_canvas_imageframe_get_arg(GtkObject* object, GtkArg* arg, guint arg_id) ;

static void gtk_canvas_imageframe_update(GtkCanvasItem *item, double *affine, ArtSVP *clip_path, int flags) ;
static void gtk_canvas_imageframe_realize(GtkCanvasItem *item) ;
static void gtk_canvas_imageframe_unrealize(GtkCanvasItem *item) ;
static void gtk_canvas_imageframe_draw(GtkCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height) ;
static double gtk_canvas_imageframe_point(GtkCanvasItem *item, double x, double y, int cx, int cy, GtkCanvasItem **actual_item) ;
static void gtk_canvas_imageframe_translate(GtkCanvasItem *item, double dx, double dy) ;
static void gtk_canvas_imageframe_bounds(GtkCanvasItem *item, double *x1, double *y1, double *x2, double *y2) ;
static void gtk_canvas_imageframe_render(GtkCanvasItem *item, GtkCanvasBuf *buf) ;

static GtkCanvasItemClass *parent_class;


GtkType
gtk_canvas_imageframe_get_type (void)
{
	static GtkType imageframe_type = 0;

	if (!imageframe_type) {
		GtkTypeInfo imageframe_info = {
			"GtkCanvasImageFrame",
			sizeof (GtkCanvasImageFrame),
			sizeof (GtkCanvasImageFrameClass),
			(GtkClassInitFunc) gtk_canvas_imageframe_class_init,
			(GtkObjectInitFunc) gtk_canvas_imageframe_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		imageframe_type = gtk_type_unique (gtk_canvas_item_get_type (), &imageframe_info);
	}

	return imageframe_type;
}

static void
gtk_canvas_imageframe_class_init (GtkCanvasImageFrameClass *class)
{
	GtkObjectClass *object_class;
	GtkCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GtkCanvasItemClass *) class;

	parent_class = gtk_type_class (gtk_canvas_item_get_type ());

	gtk_object_add_arg_type ("GtkCanvasImageFrame::pixbuf", GTK_TYPE_BOXED, GTK_ARG_WRITABLE, ARG_PIXBUF);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::x", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::y", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::width", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_WIDTH);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::drawwidth", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_DRAWWIDTH);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::height", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_HEIGHT);
	gtk_object_add_arg_type ("GtkCanvasImageFrame::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);

	object_class->destroy = gtk_canvas_imageframe_destroy;
	object_class->set_arg = gtk_canvas_imageframe_set_arg;
	object_class->get_arg = gtk_canvas_imageframe_get_arg;

	item_class->update = gtk_canvas_imageframe_update;
	item_class->realize = gtk_canvas_imageframe_realize;
	item_class->unrealize = gtk_canvas_imageframe_unrealize;
	item_class->draw = gtk_canvas_imageframe_draw;
	item_class->point = gtk_canvas_imageframe_point;
	item_class->translate = gtk_canvas_imageframe_translate;
	item_class->bounds = gtk_canvas_imageframe_bounds;
	item_class->render = gtk_canvas_imageframe_render;
}

static void
gtk_canvas_imageframe_init (GtkCanvasImageFrame *image)
{
	image->x = 0.0;
	image->y = 0.0;
	image->width = 0.0;
	image->height = 0.0;
	image->drawwidth = 0.0;
	image->anchor = GTK_ANCHOR_CENTER;
	GTK_CANVAS_ITEM(image)->object.flags |= GTK_CANVAS_ITEM_NO_AUTO_REDRAW;
}

static void
gtk_canvas_imageframe_destroy (GtkObject *object)
{
	GtkCanvasImageFrame *image;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTK_CANVAS_IS_CANVAS_IMAGEFRAME (object));

	image = GTK_CANVAS_IMAGEFRAME (object);
	
	image->cwidth = 0;
	image->cheight = 0;

	if (image->pixbuf)
	{
		art_pixbuf_free (image->pixbuf);
		image->pixbuf = NULL;
	}

	if(GTK_OBJECT_CLASS (parent_class)->destroy)
	{
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

/* Get's the image bounds expressed as item-relative coordinates. */
static void
get_bounds_item_relative (GtkCanvasImageFrame *image, double *px1, double *py1, double *px2, double *py2)
{
	GtkCanvasItem *item;
	double x, y;

	item = GTK_CANVAS_ITEM (image);

	/* Get item coordinates */

	x = image->x;
	y = image->y;

	/* Anchor image */

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= image->width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= image->width;
		break;
	}

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= image->height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= image->height;
		break;
	}

	/* Bounds */

	*px1 = x;
	*py1 = y;
	*px2 = x + image->width;
	*py2 = y + image->height;
}

static void
get_bounds (GtkCanvasImageFrame *image, double *px1, double *py1, double *px2, double *py2)
{
	GtkCanvasItem *item;
	double i2c[6];
	ArtDRect i_bbox, c_bbox;

	item = GTK_CANVAS_ITEM (image);

	gtk_canvas_item_i2c_affine (item, i2c);

	get_bounds_item_relative (image, &i_bbox.x0, &i_bbox.y0, &i_bbox.x1, &i_bbox.y1);
	art_drect_affine_transform (&c_bbox, &i_bbox, i2c);

	/* add a fudge factor */
	*px1 = c_bbox.x0 - 1;
	*py1 = c_bbox.y0 - 1;
	*px2 = c_bbox.x1 + 1;
	*py2 = c_bbox.y1 + 1;
}

/* deprecated */
static void
recalc_bounds (GtkCanvasImageFrame *image)
{
	GtkCanvasItem *item;

	item = GTK_CANVAS_ITEM (image);

	get_bounds (image, &item->x1, &item->y1, &item->x2, &item->y2);

	item->x1 = image->cx;
	item->y1 = image->cy;
	item->x2 = image->cx + image->cwidth;
	item->y2 = image->cy + image->cheight;

	gtk_canvas_group_child_bounds (GTK_CANVAS_GROUP (item->parent), item);
}

static void
gtk_canvas_imageframe_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GtkCanvasItem *item;
	GtkCanvasImageFrame *image;
	int update;
	int calc_bounds;

	item = GTK_CANVAS_ITEM (object);
	image = GTK_CANVAS_IMAGEFRAME (object);

	update = FALSE;
	calc_bounds = FALSE;

	switch (arg_id) {
	case ARG_PIXBUF:
		if (item->canvas->aa && GTK_VALUE_BOXED (*arg)) {
			if (image->pixbuf != NULL)
				art_pixbuf_free (image->pixbuf);
			image->pixbuf = GTK_VALUE_BOXED (*arg);
		}
		update = TRUE;
		break;

	case ARG_X:
		image->x = GTK_VALUE_DOUBLE (*arg);
		update = TRUE;
		break;

	case ARG_Y:
		image->y = GTK_VALUE_DOUBLE (*arg);
		update = TRUE;
		break;

	case ARG_WIDTH:
		image->width = fabs (GTK_VALUE_DOUBLE (*arg));
		update = TRUE;
		break;

	case ARG_HEIGHT:
		image->height = fabs (GTK_VALUE_DOUBLE (*arg));
		update = TRUE;
		break;
		
	case ARG_DRAWWIDTH:
		image->drawwidth = fabs (GTK_VALUE_DOUBLE (*arg));
		update = TRUE;
		break;

	case ARG_ANCHOR:
		image->anchor = GTK_VALUE_ENUM (*arg);
		update = TRUE;
		break;

	default:
		break;
	}

#ifdef OLD_XFORM
	if (update)
		(* GTK_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);

	if (calc_bounds)
		recalc_bounds (image);
#else
	if (update)
		gtk_canvas_item_request_update (item);
#endif
}

static void
gtk_canvas_imageframe_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME (object);

	switch (arg_id) {

	case ARG_X:
		GTK_VALUE_DOUBLE (*arg) = image->x;
		break;

	case ARG_Y:
		GTK_VALUE_DOUBLE (*arg) = image->y;
		break;

	case ARG_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = image->width;
		break;

	case ARG_HEIGHT:
		GTK_VALUE_DOUBLE (*arg) = image->height;
		break;
		
	case ARG_DRAWWIDTH:
		GTK_VALUE_DOUBLE (*arg) = image->drawwidth;
		break;

	case ARG_ANCHOR:
		GTK_VALUE_ENUM (*arg) = image->anchor;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gtk_canvas_imageframe_update (GtkCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GtkCanvasImageFrame *image;
	ArtDRect i_bbox, c_bbox;
	int w = 0;
	int h = 0;

	image = GTK_CANVAS_IMAGEFRAME (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	/* only works for non-rotated, non-skewed transforms */
	image->cwidth = (int) (image->width * affine[0] + 0.5);
	image->cheight = (int) (image->height * affine[3] + 0.5);

	if (image->pixbuf) {
		image->need_recalc = TRUE ;
	}

#ifdef OLD_XFORM
	recalc_bounds (image);
#else
	get_bounds_item_relative (image, &i_bbox.x0, &i_bbox.y0, &i_bbox.x1, &i_bbox.y1);
	art_drect_affine_transform (&c_bbox, &i_bbox, affine);

	/* these values only make sense in the non-rotated, non-skewed case */
	image->cx = c_bbox.x0;
	image->cy = c_bbox.y0;

	/* add a fudge factor */
	c_bbox.x0--;
	c_bbox.y0--;
	c_bbox.x1++;
	c_bbox.y1++;

	gtk_canvas_update_bbox (item, c_bbox.x0, c_bbox.y0, c_bbox.x1, c_bbox.y1);

	if (image->pixbuf) {
		w = image->pixbuf->width;
		h = image->pixbuf->height;
	}

	image->affine[0] = (affine[0] * image->width) / w;
	image->affine[1] = (affine[1] * image->height) / h;
	image->affine[2] = (affine[2] * image->width) / w;
	image->affine[3] = (affine[3] * image->height) / h;
	image->affine[4] = i_bbox.x0 * affine[0] + i_bbox.y0 * affine[2] + affine[4];
	image->affine[5] = i_bbox.x0 * affine[1] + i_bbox.y0 * affine[3] + affine[5];
	
#endif
}

static void
gtk_canvas_imageframe_realize (GtkCanvasItem *item)
{
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

}

static void
gtk_canvas_imageframe_unrealize (GtkCanvasItem *item)
{
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME(item);

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

static void
recalc_if_needed (GtkCanvasImageFrame *image)
{}

static void
gtk_canvas_imageframe_draw (GtkCanvasItem *item, GdkDrawable *drawable,
			 int x, int y, int width, int height)
{
	fprintf(stderr, "please don't use the CanvasImageFrame item in a non-aa Canvas\n") ;
	abort() ;
}

static double
gtk_canvas_imageframe_point (GtkCanvasItem *item, double x, double y,
			  int cx, int cy, GtkCanvasItem **actual_item)
{
	GtkCanvasImageFrame *image;
	int x1, y1, x2, y2;
	int dx, dy;

	image = GTK_CANVAS_IMAGEFRAME (item);

	*actual_item = item;

	recalc_if_needed (image);

	x1 = image->cx - item->canvas->close_enough;
	y1 = image->cy - item->canvas->close_enough;
	x2 = image->cx + image->cwidth - 1 + item->canvas->close_enough;
	y2 = image->cy + image->cheight - 1 + item->canvas->close_enough;

	/* Hard case: is point inside image's gravity region? */

	//if ((cx >= x1) && (cy >= y1) && (cx <= x2) && (cy <= y2))
		//return dist_to_mask (image, cx, cy) / item->canvas->pixels_per_unit;

	/* Point is outside image */

	x1 += item->canvas->close_enough;
	y1 += item->canvas->close_enough;
	x2 -= item->canvas->close_enough;
	y2 -= item->canvas->close_enough;

	if (cx < x1)
		dx = x1 - cx;
	else if (cx > x2)
		dx = cx - x2;
	else
		dx = 0;

	if (cy < y1)
		dy = y1 - cy;
	else if (cy > y2)
		dy = cy - y2;
	else
		dy = 0;

	return sqrt (dx * dx + dy * dy) / item->canvas->pixels_per_unit;
}

static void
gtk_canvas_imageframe_translate (GtkCanvasItem *item, double dx, double dy)
{
#ifdef OLD_XFORM
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME (item);

	image->x += dx;
	image->y += dy;

	recalc_bounds (image);
#endif
}

static void
gtk_canvas_imageframe_bounds (GtkCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME (item);

	*x1 = image->x;
	*y1 = image->y;

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		*x1 -= image->width / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		*x1 -= image->width;
		break;
	}

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		*y1 -= image->height / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		*y1 -= image->height;
		break;
	}

	*x2 = *x1 + image->width;
	*y2 = *y1 + image->height;
}

static void
gtk_canvas_imageframe_render      (GtkCanvasItem *item, GtkCanvasBuf *buf)
{
	GtkCanvasImageFrame *image;

	image = GTK_CANVAS_IMAGEFRAME (item);

        gtk_canvas_buf_ensure_buf (buf);

#ifdef VERBOSE
	{
		char str[128];
		art_affine_to_string (str, image->affine);
		g_print ("gtk_canvas_imageframe_render %s\n", str);
	}
#endif

	art_rgb_pixbuf_affine (buf->buf,
			buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
			buf->buf_rowstride,
			image->pixbuf,
			image->affine,
			ART_FILTER_NEAREST, NULL);

	buf->is_bg = 0;
}
