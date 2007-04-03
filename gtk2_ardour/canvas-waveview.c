/*
     Copyright (C) 2000-2002 Paul Davis 

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

     $Id$
*/

#include <stdio.h>
#include <math.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <string.h>
#include <limits.h>

#include <ardour/dB.h>

#include "logmeter.h"
#include "canvas-waveview.h"
#include "rgb_macros.h"


extern void c_stacktrace();

enum {
	 PROP_0,
	 PROP_DATA_SRC,
	 PROP_CHANNEL,
	 PROP_LENGTH_FUNCTION,
	 PROP_SOURCEFILE_LENGTH_FUNCTION,
	 PROP_PEAK_FUNCTION,
	 PROP_GAIN_FUNCTION,
	 PROP_GAIN_SRC,
	 PROP_CACHE,
	 PROP_CACHE_UPDATER,
	 PROP_SAMPLES_PER_UNIT,
	 PROP_AMPLITUDE_ABOVE_AXIS,
	 PROP_X,
	 PROP_Y,
	 PROP_HEIGHT,
	 PROP_WAVE_COLOR,
	 PROP_CLIP_COLOR,
	 PROP_ZERO_COLOR,
	 PROP_RECTIFIED,
	 PROP_REGION_START,
	 PROP_LOGSCALED,
};

static void gnome_canvas_waveview_class_init     (GnomeCanvasWaveViewClass *class);

static void gnome_canvas_waveview_init           (GnomeCanvasWaveView      *waveview);

static void gnome_canvas_waveview_destroy        (GtkObject            *object);

static void gnome_canvas_waveview_set_property   (GObject        *object,
						   guint           prop_id,
						   const GValue   *value,
						   GParamSpec     *pspec);
static void gnome_canvas_waveview_get_property   (GObject        *object,
						   guint           prop_id,
						   GValue         *value,
						   GParamSpec     *pspec);

static void   gnome_canvas_waveview_update       (GnomeCanvasItem *item,
						   double          *affine,
						   ArtSVP          *clip_path,
						   int              flags);

static void   gnome_canvas_waveview_bounds       (GnomeCanvasItem *item,
						   double          *x1,
						   double          *y1,
						   double          *x2,
						   double          *y2);

static double gnome_canvas_waveview_point        (GnomeCanvasItem  *item,
						   double            x,
						   double            y,
						   int               cx,
						   int               cy,
						   GnomeCanvasItem **actual_item);

static void gnome_canvas_waveview_render         (GnomeCanvasItem *item,
						   GnomeCanvasBuf  *buf);

static void gnome_canvas_waveview_draw           (GnomeCanvasItem *item,
						   GdkDrawable     *drawable,
						   int              x,
						   int              y,
						   int              w,
						   int              h);

static void gnome_canvas_waveview_set_data_src   (GnomeCanvasWaveView *,
						   void *);

static void gnome_canvas_waveview_set_channel    (GnomeCanvasWaveView *,
						   guint32);

static gint32 gnome_canvas_waveview_ensure_cache (GnomeCanvasWaveView *waveview,
						   gulong               start_sample,
						   gulong               end_sample);

static GnomeCanvasItemClass *parent_class;

GType
gnome_canvas_waveview_get_type (void)
{
	 static GType waveview_type;

	 if (!waveview_type) {
		 static const GTypeInfo object_info = {
			 sizeof (GnomeCanvasWaveViewClass),
			 (GBaseInitFunc) NULL,
			 (GBaseFinalizeFunc) NULL,
			 (GClassInitFunc) gnome_canvas_waveview_class_init,
			 (GClassFinalizeFunc) NULL,
			 NULL,			/* class_data */
			 sizeof (GnomeCanvasWaveView),
			 0,			/* n_preallocs */
			 (GInstanceInitFunc) gnome_canvas_waveview_init,
			 NULL			/* value_table */
		 };

		 waveview_type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM, "GnomeCanvasWaveView",
							 &object_info, 0);
	 }

	 return waveview_type;
 }

static void
gnome_canvas_waveview_class_init (GnomeCanvasWaveViewClass *class)
{
	 GObjectClass *gobject_class;
	 GtkObjectClass *object_class;
	 GnomeCanvasItemClass *item_class;

	 gobject_class = (GObjectClass *) class;
	 object_class = (GtkObjectClass *) class;
	 item_class = (GnomeCanvasItemClass *) class;

	 parent_class = g_type_class_peek_parent (class);

	 gobject_class->set_property = gnome_canvas_waveview_set_property;
	 gobject_class->get_property = gnome_canvas_waveview_get_property;

	 g_object_class_install_property
		 (gobject_class,
		  PROP_DATA_SRC,
		  g_param_spec_pointer ("data_src", NULL, NULL,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_CHANNEL,
		  g_param_spec_uint ("channel", NULL, NULL,
				     0, G_MAXUINT, 0,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_LENGTH_FUNCTION,
		  g_param_spec_pointer ("length_function", NULL, NULL,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
                (gobject_class,
                 PROP_SOURCEFILE_LENGTH_FUNCTION,
                 g_param_spec_pointer ("sourcefile_length_function", NULL, NULL,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_PEAK_FUNCTION,
		  g_param_spec_pointer ("peak_function", NULL, NULL,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_GAIN_FUNCTION,
		  g_param_spec_pointer ("gain_function", NULL, NULL,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
                 PROP_GAIN_SRC,
                 g_param_spec_pointer ("gain_src", NULL, NULL,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	
	 g_object_class_install_property
		 (gobject_class,
		  PROP_CACHE,
		  g_param_spec_pointer ("cache", NULL, NULL,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_CACHE_UPDATER,
                 g_param_spec_boolean ("cache_updater", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_SAMPLES_PER_UNIT,
		  g_param_spec_double ("samples_per_unit", NULL, NULL,
				       0.0, G_MAXDOUBLE, 0.0,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_AMPLITUDE_ABOVE_AXIS,
		  g_param_spec_double ("amplitude_above_axis", NULL, NULL,
				       0.0, G_MAXDOUBLE, 0.0,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_X,
		  g_param_spec_double ("x", NULL, NULL,
				       0.0, G_MAXDOUBLE, 0.0,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_Y,
		  g_param_spec_double ("y", NULL, NULL,
				       0.0, G_MAXDOUBLE, 0.0,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_HEIGHT,
		  g_param_spec_double ("height", NULL, NULL,
				       0.0, G_MAXDOUBLE, 0.0,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_WAVE_COLOR,
		  g_param_spec_uint ("wave_color", NULL, NULL,
				     0, G_MAXUINT, 0,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_CLIP_COLOR,
		  g_param_spec_uint ("clip_color", NULL, NULL,
				     0, G_MAXUINT, 0,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_ZERO_COLOR,
		  g_param_spec_uint ("zero_color", NULL, NULL,
				     0, G_MAXUINT, 0,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_RECTIFIED,
		  g_param_spec_boolean ("rectified", NULL, NULL,
					FALSE,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));

	 g_object_class_install_property
		 (gobject_class,
		  PROP_LOGSCALED,
		  g_param_spec_boolean ("logscaled", NULL, NULL,
					FALSE,
					(G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 g_object_class_install_property
		 (gobject_class,
		  PROP_REGION_START,
		  g_param_spec_uint ("region_start", NULL, NULL,
				     0, G_MAXUINT, 0,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
	 
	 object_class->destroy = gnome_canvas_waveview_destroy;
	 
	 item_class->update = gnome_canvas_waveview_update;
	 item_class->bounds = gnome_canvas_waveview_bounds;
	 item_class->point = gnome_canvas_waveview_point;
	 item_class->render = gnome_canvas_waveview_render;
	 item_class->draw = gnome_canvas_waveview_draw;
}

GnomeCanvasWaveViewCache*
gnome_canvas_waveview_cache_new ()
{
	GnomeCanvasWaveViewCache *c;

	c = g_malloc (sizeof (GnomeCanvasWaveViewCache));

	c->allocated = 2048;
	c->data = g_malloc (sizeof (GnomeCanvasWaveViewCacheEntry) * c->allocated);
	c->data_size = 0;
	c->start = 0;
	c->end = 0;

	return c;
}

void
gnome_canvas_waveview_cache_destroy (GnomeCanvasWaveViewCache* cache)
{
	g_free (cache->data);
	g_free (cache);
}

static void
gnome_canvas_waveview_init (GnomeCanvasWaveView *waveview)
{
	waveview->x = 0.0;
	waveview->y = 0.0;
	waveview->cache = 0;
	waveview->cache_updater = FALSE;
	waveview->data_src = NULL;
	waveview->channel = 0;
	waveview->peak_function = NULL;
	waveview->length_function = NULL;
	waveview->sourcefile_length_function = NULL;
	waveview->gain_curve_function = NULL;
	waveview->gain_src = NULL;
	waveview->rectified = FALSE;
	waveview->logscaled = FALSE;
	waveview->region_start = 0;
	waveview->samples_per_unit = 1.0;
	waveview->amplitude_above_axis = 1.0;
	waveview->height = 100.0;
	waveview->screen_width = gdk_screen_width ();
	waveview->reload_cache_in_render = FALSE;

 	waveview->wave_color = RGBA_TO_UINT(44,35,126,255);
 	waveview->clip_color = RGBA_TO_UINT(44,0,0,100);
 	waveview->zero_color = RGBA_TO_UINT(44,0,128,100);
}

static void
gnome_canvas_waveview_destroy (GtkObject *object)
{
	GnomeCanvasWaveView *waveview;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_WAVEVIEW (object));

	waveview = GNOME_CANVAS_WAVEVIEW (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

#define DEBUG_CACHE 0
#undef CACHE_MEMMOVE_OPTIMIZATION

static gint32
gnome_canvas_waveview_ensure_cache (GnomeCanvasWaveView *waveview, gulong start_sample, gulong end_sample)
{
	gulong required_cache_entries;
	gulong rf1, rf2,rf3, required_frames;
	gulong new_cache_start, new_cache_end;
	gulong half_width;
	gulong npeaks;
	gulong offset;
	gulong ostart;
	gulong copied;
	GnomeCanvasWaveViewCache *cache;
	float* gain;
#ifdef CACHE_MEMMOVE_OPTIMIZATION
	gulong present_frames;
	gulong present_entries;
#endif

	cache = waveview->cache;

	start_sample = start_sample + waveview->region_start;
	end_sample = end_sample + waveview->region_start;
#if DEBUG_CACHE
	// printf("waveview->region_start == %lu\n",waveview->region_start);
	// c_stacktrace ();
	printf ("\n\n=> 0x%x cache @ 0x%x range: %lu - %lu request: %lu - %lu (%lu frames)\n", 
		waveview, cache,
		cache->start, cache->end,
		start_sample, end_sample, end_sample - start_sample);
#endif
		
	if (cache->start <= start_sample && cache->end >= end_sample) {
#if DEBUG_CACHE
		// printf ("0x%x: cache hit for %lu-%lu (cache holds: %lu-%lu\n",
		// waveview, start_sample, end_sample, cache->start, cache->end);
#endif
		goto out;
	}

	/* make sure the cache is at least twice as wide as the screen width, and put the start sample
	   in the middle, ensuring that we cover the end_sample. 
	*/

	/* Note the assumption that we have a 1:1 units:pixel ratio for the canvas. Its everywhere ... */
	
	half_width = (gulong) floor ((waveview->screen_width * waveview->samples_per_unit)/2.0 + 0.5);
	
	if (start_sample < half_width) {
		new_cache_start = 0;
	} else {
		new_cache_start = start_sample - half_width;
	}

	/* figure out how many frames we want */

	rf1 = end_sample - start_sample + 1;
	rf2 = (gulong) floor ((waveview->screen_width * waveview->samples_per_unit * 2.0f));
	required_frames = MAX(rf1,rf2);

	/* but make sure it doesn't extend beyond the end of the source material */

	rf3 = (gulong) (waveview->sourcefile_length_function (waveview->data_src, waveview->samples_per_unit)) + 1;
	if (rf3 < new_cache_start) {
		rf3 = 0;
	} else {
		rf3 -= new_cache_start;
	}

#if DEBUG_CACHE
	fprintf (stderr, "AVAILABLE FRAMES = %lu of %lu, start = %lu, sstart = %lu, cstart = %lu\n", 
		 rf3, waveview->sourcefile_length_function (waveview->data_src, waveview->samples_per_unit),
		 waveview->region_start, start_sample, new_cache_start);
#endif

	required_frames = MIN(required_frames,rf3);

	new_cache_end = new_cache_start + required_frames - 1;

	required_cache_entries = (gulong) floor (required_frames / waveview->samples_per_unit );

#if DEBUG_CACHE
	fprintf (stderr, "new cache = %lu - %lu\n", new_cache_start, new_cache_end);
	fprintf(stderr,"required_cach_entries = %lu, samples_per_unit = %f req frames = %lu\n",
		required_cache_entries,waveview->samples_per_unit, required_frames);
#endif

	if (required_cache_entries > cache->allocated) {
		cache->data = g_realloc (cache->data, sizeof (GnomeCanvasWaveViewCacheEntry) * required_cache_entries);
		cache->allocated = required_cache_entries;
		// cache->start = 0;
		// cache->end = 0;
	}

	ostart = new_cache_start;

#ifdef CACHE_MEMMOVE_OPTIMIZATION
	
	/* data is not entirely in the cache, so go fetch it, making sure to fill the cache */

	/* some of the required cache entries are in the cache, but in the wrong
	   locations. use memmove to fix this.
	*/

	if (cache->start < new_cache_start && new_cache_start < cache->end) {
		
		/* case one: the common area is at the end of the existing cache. move it 
		   to the beginning of the cache, and set up to refill whatever remains.
		   
		   
			   wv->cache_start                                        wv->cache_end
			   |-------------------------------------------------------| cache
			                                       |--------------------------------| requested
			                                       <------------------->
			                                             "present"
			                                    new_cache_start                      new_cache_end       
		*/
				

		present_frames = cache->end - new_cache_start;
		present_entries = (gulong) floor (present_frames / waveview->samples_per_unit);

#if DEBUG_CACHE		
		fprintf (stderr, "existing material at end of current cache, move to start of new cache\n"
			 "\tcopy from %lu to start\n", cache->data_size - present_entries);
#endif

		memmove (&cache->data[0],
			 &cache->data[cache->data_size - present_entries],
			 present_entries * sizeof (GnomeCanvasWaveViewCacheEntry));
		
#if DEBUG_CACHE
		fprintf (stderr, "satisfied %lu of %lu frames, offset = %lu, will start at %lu (ptr = 0x%x)\n",
			 present_frames, required_frames, present_entries, new_cache_start + present_entries,
			 cache->data + present_entries);
#endif

		copied = present_entries;
		offset = present_entries;
		new_cache_start += present_frames;
		required_frames -= present_frames;

	} else if (new_cache_end > cache->start && new_cache_end < cache->end) {

		/* case two: the common area lives at the beginning of the existing cache. 
		   
                                            wv->cache_start                                      wv->cache_end
			                     |-----------------------------------------------------|
                              |--------------------------------|
                                             <----------------->
					        "present"

                             new_cache_start                      new_cache_end
		*/
		
		present_frames = new_cache_end - cache->start;
		present_entries = (gulong) floor (present_frames / waveview->samples_per_unit);

		memmove (&cache->data[cache->data_size - present_entries],
			 &cache->data[0],
			 present_entries * sizeof (GnomeCanvasWaveViewCacheEntry));
		
#if DEBUG_CACHE		
		fprintf (stderr, "existing material at start of current cache, move to start of end cache\n");
#endif

#if DEBUG_CACHE
		fprintf (stderr, "satisfied %lu of %lu frames, offset = %lu, will start at %lu (ptr = 0x%x)\n",
			 present_entries, required_frames, present_entries, new_cache_start + present_entries,
			 cache->data + present_entries);
#endif

		copied = present_entries;
		offset = 0;
		required_frames -= present_frames;

		
	} else {
		copied = 0;
		offset = 0;

	}

#else
	copied = 0;
	offset = 0;

#endif /* CACHE_MEMMOVE_OPTIMIZATION */

//	fprintf(stderr,"length == %lu\n",waveview->length_function (waveview->data_src));
//	required_frames = MIN (waveview->length_function (waveview->data_src) - new_cache_start, required_frames);

	npeaks = (gulong) floor (required_frames / waveview->samples_per_unit);
	required_frames = npeaks * waveview->samples_per_unit;

#if DEBUG_CACHE


	printf ("requesting %lu/%f to cover %lu-%lu at %f spu (request was %lu-%lu) into cache + %lu\n",
		required_frames, required_frames/waveview->samples_per_unit, new_cache_start, new_cache_end,
		waveview->samples_per_unit, start_sample, end_sample, offset);
#endif

#if DEBUG_CACHE
//	printf ("cache holds %lu entries, requesting %lu to cover %lu-%lu (request was %lu-%lu)\n",
//		cache->data_size, npeaks, new_cache_start, new_cache_end,
//		start_sample, end_sample);
#endif

	if (required_frames) {
		waveview->peak_function (waveview->data_src, npeaks, new_cache_start, required_frames, cache->data + offset, waveview->channel,waveview->samples_per_unit);

		/* take into account any copied peaks */
		
		npeaks += copied;
	} else {
		npeaks = copied;
	}

	if (npeaks < cache->allocated) {
#if DEBUG_CACHE
		fprintf (stderr, "zero fill cache for %lu at %lu\n", cache->allocated - npeaks, npeaks);
#endif
		memset (&cache->data[npeaks], 0, sizeof (GnomeCanvasWaveViewCacheEntry) * (cache->allocated - npeaks));
		cache->data_size = npeaks;
	} else {
		cache->data_size = cache->allocated;
	}

	if (waveview->gain_curve_function) {
		guint32 n;

		gain = (float*) malloc (sizeof (float) * cache->data_size);

		waveview->gain_curve_function (waveview->gain_src, new_cache_start, new_cache_end, gain, cache->data_size);

		for (n = 0; n < cache->data_size; ++n) {
			cache->data[n].min *= gain[n];
			cache->data[n].max *= gain[n];
		}

		free (gain);
	
	}

	/* do optional log scaling.  this implementation is not particularly efficient */
	
	if (waveview->logscaled) {
		guint32 n;
		GnomeCanvasWaveViewCacheEntry* buf = cache->data;
		
		for (n = 0; n < cache->data_size; ++n) {

			if (buf[n].max > 0.0f) {
				buf[n].max = alt_log_meter(coefficient_to_dB(buf[n].max));
			} else if (buf[n].max < 0.0f) {
				buf[n].max = -alt_log_meter(coefficient_to_dB(-buf[n].max));
			}
			
			if (buf[n].min > 0.0f) {
				buf[n].min = alt_log_meter(coefficient_to_dB(buf[n].min));
			} else if (buf[n].min < 0.0f) {
				buf[n].min = -alt_log_meter(coefficient_to_dB(-buf[n].min));
			}
		}
	}

	cache->start = ostart;
	cache->end = new_cache_end;

  out:
#if DEBUG_CACHE
	fprintf (stderr, "return cache index = %d\n", 
		 (gint32) floor ((((double) (start_sample - cache->start)) / waveview->samples_per_unit) + 0.5));
#endif
	return (gint32) floor ((((double) (start_sample - cache->start)) / waveview->samples_per_unit) + 0.5);

}

void
gnome_canvas_waveview_set_data_src (GnomeCanvasWaveView *waveview, void *data_src)
{

	if (waveview->cache_updater) {
		if (waveview->data_src == data_src) {
			waveview->reload_cache_in_render = TRUE;
			return;
		}
	
		waveview->cache->start  = 0;
		waveview->cache->end = 0;
	}

	waveview->data_src = data_src;
}

void
gnome_canvas_waveview_set_channel (GnomeCanvasWaveView *waveview, guint32 chan)
{
	if (waveview->channel == chan) {
		return;
	}
	
	waveview->channel = chan;
}

static void 
gnome_canvas_waveview_reset_bounds (GnomeCanvasItem *item)

{
	double x1, x2, y1, y2;
	ArtPoint i1, i2;
	ArtPoint w1, w2;
	int Ix1, Ix2, Iy1, Iy2;
	double i2w[6];

	gnome_canvas_waveview_bounds (item, &x1, &y1, &x2, &y2);

	i1.x = x1;
	i1.y = y1;
	i2.x = x2;
	i2.y = y2;

	gnome_canvas_item_i2w_affine (item, i2w);
	art_affine_point (&w1, &i1, i2w);
	art_affine_point (&w2, &i2, i2w);

	Ix1 = (int) rint(w1.x);
	Ix2 = (int) rint(w2.x);
	Iy1 = (int) rint(w1.y);
	Iy2 = (int) rint(w2.y);

	gnome_canvas_update_bbox (item, Ix1, Iy1, Ix2, Iy2);
}

/* 
 * CANVAS CALLBACKS 
 */

static void
gnome_canvas_waveview_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)

{
	GnomeCanvasItem *item;
	GnomeCanvasWaveView *waveview;
	int redraw = FALSE;
	int calc_bounds = FALSE;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_WAVEVIEW (object));

	item = GNOME_CANVAS_ITEM (object);
	waveview = GNOME_CANVAS_WAVEVIEW (object);

	switch (prop_id) {
	case PROP_DATA_SRC:
		gnome_canvas_waveview_set_data_src (waveview, g_value_get_pointer(value));
		redraw = TRUE;
		break;

	case PROP_CHANNEL:
		gnome_canvas_waveview_set_channel (waveview, g_value_get_uint(value));
		redraw = TRUE;
		break;

	case PROP_LENGTH_FUNCTION:
		waveview->length_function = g_value_get_pointer(value);
		redraw = TRUE;
		break;
	case PROP_SOURCEFILE_LENGTH_FUNCTION:
		waveview->sourcefile_length_function = g_value_get_pointer(value);
		redraw = TRUE;
		break;

	case PROP_PEAK_FUNCTION:
		waveview->peak_function = g_value_get_pointer(value);
		redraw = TRUE;
		break;

	case PROP_GAIN_FUNCTION:
		waveview->gain_curve_function = g_value_get_pointer(value);
		redraw = TRUE;
		break;

	case PROP_GAIN_SRC:
		waveview->gain_src = g_value_get_pointer(value);
		if (waveview->cache_updater) {
			waveview->cache->start = 0;
			waveview->cache->end = 0;
		}
		redraw = TRUE;
		calc_bounds = TRUE;
		break;

	case PROP_CACHE:
		waveview->cache = g_value_get_pointer(value);
		redraw = TRUE;
		break;


	case PROP_CACHE_UPDATER:
		waveview->cache_updater = g_value_get_boolean(value);
		redraw = TRUE;
		break;

	case PROP_SAMPLES_PER_UNIT:
		if ((waveview->samples_per_unit = g_value_get_double(value)) < 1.0) {
			waveview->samples_per_unit = 1.0;
		}
		if (waveview->cache_updater) {
			waveview->cache->start = 0;
			waveview->cache->end = 0;
		}
		redraw = TRUE;
		calc_bounds = TRUE;
		break;

	case PROP_AMPLITUDE_ABOVE_AXIS:
		waveview->amplitude_above_axis = g_value_get_double(value);
		redraw = TRUE;
		break;

	case PROP_X:
	        if (waveview->x != g_value_get_double (value)) {
		        waveview->x = g_value_get_double (value);
			calc_bounds = TRUE;
		}
		break;

	case PROP_Y:
	        if (waveview->y != g_value_get_double (value)) {
		        waveview->y = g_value_get_double (value);
			calc_bounds = TRUE;
		}
		break;

	case PROP_HEIGHT:
	        if (waveview->height != fabs (g_value_get_double (value))) {
		        waveview->height = fabs (g_value_get_double (value));
			redraw = TRUE;
		}
		break;

	case PROP_WAVE_COLOR:
		if (waveview->wave_color != g_value_get_uint(value)) {
		        waveview->wave_color = g_value_get_uint(value);
			redraw = TRUE;
		}
		break;

	case PROP_CLIP_COLOR:
		if (waveview->clip_color != g_value_get_uint(value)) {
		        waveview->clip_color = g_value_get_uint(value);
			redraw = TRUE;
		}
		break;

	case PROP_ZERO_COLOR:
		if (waveview->zero_color != g_value_get_uint(value)) {
		        waveview->zero_color = g_value_get_uint(value);
			redraw = TRUE;
		}
		break;

	case PROP_RECTIFIED:
		if (waveview->rectified != g_value_get_boolean(value)) {
			waveview->rectified = g_value_get_boolean(value);
			redraw = TRUE;
		}
		break;
	case PROP_LOGSCALED:
		if (waveview->logscaled != g_value_get_boolean(value)) {
			waveview->logscaled = g_value_get_boolean(value);
			if (waveview->cache_updater) {
				waveview->cache->start = 0;
				waveview->cache->end = 0;
			}
			redraw = TRUE;
			calc_bounds = TRUE;
		}
		break;
	case PROP_REGION_START:
		waveview->region_start = g_value_get_uint(value);
		redraw = TRUE;
		calc_bounds = TRUE;
		break;


	default:
		break;
	}

	if (calc_bounds) {
		gnome_canvas_waveview_reset_bounds (item);
	}

	if (redraw) {
		gnome_canvas_item_request_update (item);
	}

}

static void
gnome_canvas_waveview_get_property (GObject      *object,
				    guint         prop_id,
				    GValue       *value,
				    GParamSpec   *pspec)
{
	
   
	g_return_if_fail (object != NULL);
        g_return_if_fail (GNOME_IS_CANVAS_WAVEVIEW (object));

	GnomeCanvasWaveView *waveview = GNOME_CANVAS_WAVEVIEW (object);

	switch (prop_id) {
	case PROP_DATA_SRC:
	        g_value_set_pointer(value, waveview->data_src);
		break;

	case PROP_CHANNEL:
	        g_value_set_uint(value, waveview->channel);
		break;

	case PROP_LENGTH_FUNCTION:
	        g_value_set_pointer(value, waveview->length_function);
		break;

	case PROP_SOURCEFILE_LENGTH_FUNCTION:
	        g_value_set_pointer(value, waveview->sourcefile_length_function);
		break;

	case PROP_PEAK_FUNCTION:
	        g_value_set_pointer(value, waveview->peak_function);
		break;

	case PROP_GAIN_FUNCTION:
	        g_value_set_pointer(value, waveview->gain_curve_function);
		break;

	case PROP_GAIN_SRC:
	        g_value_set_pointer(value, waveview->gain_src);
		break;

	case PROP_CACHE:
	        g_value_set_pointer(value, waveview->cache);
		break;

	case PROP_CACHE_UPDATER:
	        g_value_set_boolean(value, waveview->cache_updater);
		break;

	case PROP_SAMPLES_PER_UNIT:
	        g_value_set_double(value, waveview->samples_per_unit);
		break;

	case PROP_AMPLITUDE_ABOVE_AXIS:
	        g_value_set_double(value, waveview->amplitude_above_axis);
		break;

	case PROP_X:
		g_value_set_double (value, waveview->x);
		break;

	case PROP_Y:
		g_value_set_double (value, waveview->y);
		break;

	case PROP_HEIGHT:
		g_value_set_double (value, waveview->height);
		break;

	case PROP_WAVE_COLOR:
		g_value_set_uint (value, waveview->wave_color);
		break;

	case PROP_CLIP_COLOR:
		g_value_set_uint (value, waveview->clip_color);
		break;

	case PROP_ZERO_COLOR:
		g_value_set_uint (value, waveview->zero_color);
		break;

	case PROP_RECTIFIED:
		g_value_set_boolean (value, waveview->rectified);
		break;

	case PROP_LOGSCALED:
		g_value_set_boolean (value, waveview->logscaled);
		break;

	case PROP_REGION_START:
		g_value_set_uint (value, waveview->region_start);
		break;

	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gnome_canvas_waveview_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasWaveView *waveview;
	double x, y;

	waveview = GNOME_CANVAS_WAVEVIEW (item);

//	check_cache (waveview, "start of update");

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_waveview_reset_bounds (item);

	/* get the canvas coordinates of the view. Do NOT use affines
	   for this, because they do not round to the integer units used
	   by the canvas, resulting in subtle pixel-level errors later.
	*/

	x = waveview->x;
	y = waveview->y;

	gnome_canvas_item_i2w (item, &x, &y);
	gnome_canvas_w2c (GNOME_CANVAS(item->canvas), x, y, &waveview->bbox_ulx, &waveview->bbox_uly);

	waveview->samples = waveview->length_function (waveview->data_src);

	x = waveview->x + (waveview->samples / waveview->samples_per_unit);
	y = waveview->y + waveview->height;

	gnome_canvas_item_i2w (item, &x, &y);
	gnome_canvas_w2c (GNOME_CANVAS(item->canvas), x, y, &waveview->bbox_lrx, &waveview->bbox_lry);

	/* cache the half-height and the end point in canvas units */

	waveview->half_height = waveview->height / 2.0;

	/* parse the color */

	UINT_TO_RGBA (waveview->wave_color, &waveview->wave_r, &waveview->wave_g, &waveview->wave_b,
		      &waveview->wave_a);
	UINT_TO_RGBA (waveview->clip_color, &waveview->clip_r, &waveview->clip_g, &waveview->clip_b,
		      &waveview->clip_a);

//	check_cache (waveview, "end of update");
}

static void
gnome_canvas_waveview_render (GnomeCanvasItem *item,
			    GnomeCanvasBuf *buf)
{
	GnomeCanvasWaveView *waveview;
	gulong s1, s2;
	int clip_length = 0;
	int pymin, pymax;
	int cache_index;
	double half_height;
	int x, end, begin;
	int zbegin, zend;

	waveview = GNOME_CANVAS_WAVEVIEW (item);

//	check_cache (waveview, "start of render");

	if (parent_class->render) {
		(*parent_class->render) (item, buf);
	}

	if (buf->is_bg) {
		gnome_canvas_buf_ensure_buf (buf);
		buf->is_bg = FALSE;
	}

	begin = MAX(waveview->bbox_ulx, buf->rect.x0);

	if (begin == waveview->bbox_ulx) {
		zbegin = begin + 1;
	} else {
		zbegin = begin;
	}

	if (waveview->bbox_lrx >= 0) {
		end = MIN(waveview->bbox_lrx,buf->rect.x1);
	} else {
		end = buf->rect.x1;
	}

	if (end == waveview->bbox_lrx) {
		zend = end - 1;
	} else {
		zend = end;
	}

	if (begin == end) {
		return;
	}

	s1 = floor ((begin - waveview->bbox_ulx) * waveview->samples_per_unit) ;

	// fprintf (stderr, "0x%x begins at sample %f\n", waveview, waveview->bbox_ulx * waveview->samples_per_unit);

	if (end == waveview->bbox_lrx) {
		/* This avoids minor rounding errors when we have the
		   entire region visible.
		*/
		s2 = waveview->samples;
	} else {
		s2 = s1 + floor ((end - begin) * waveview->samples_per_unit);
	}

#if 0
	printf ("0x%x r (%d..%d)(%d..%d) bbox (%d..%d)(%d..%d)"
		" b/e %d..%d s= %lu..%lu @ %f\n",
		waveview,
		buf->rect.x0,
		buf->rect.x1,
		buf->rect.y0,
		buf->rect.y1,
		waveview->bbox_ulx,
		waveview->bbox_lrx,
		waveview->bbox_uly,
		waveview->bbox_lry,
		begin, end, s1, s2,
		waveview->samples_per_unit);
#endif

	/* now ensure that the cache is full and properly
	   positioned.
	*/

//	check_cache (waveview, "pre-ensure");

	if (waveview->cache_updater && waveview->reload_cache_in_render) {
		waveview->cache->start = 0;
		waveview->cache->end = 0;
		waveview->reload_cache_in_render = FALSE;
	}

	cache_index = gnome_canvas_waveview_ensure_cache (waveview, s1, s2);

//	check_cache (waveview, "post-ensure");

	/* 
	   Now draw each line, clipping it appropriately. The clipping
	   is done by the macros PAINT_FOO().
	*/

	half_height = waveview->half_height;

/* this makes it slightly easier to comprehend whats going on */

#define origin half_height


	for (x = begin; x < end; x++) {

		double max, min;
		int clip_max, clip_min;
		
		clip_max = 0;
		clip_min = 0;

		max = waveview->cache->data[cache_index].max;
		min = waveview->cache->data[cache_index].min;
		
		if (max >= 1.0) {
			max = 1.0;
			clip_max = 1;
		}

		if (min <= -1.0) {
			min = -1.0;
			clip_min = 1;
		}

		/* don't rectify at single-sample zoom */

		if (waveview->rectified && waveview->samples_per_unit > 1) {

			if (fabs (min) > fabs (max)) {
				max = fabs (min);
			} 

			max = max * waveview->height;

			pymax = (int) rint ((item->y1 + waveview->height - max) * item->canvas->pixels_per_unit);
			pymin = (int) rint ((item->y1 + waveview->height) * item->canvas->pixels_per_unit);

		} else {
			
			max = max * half_height;
			min = min * half_height;
			
			pymax = (int) rint ((item->y1 + origin - max) * item->canvas->pixels_per_unit);
			pymin = (int) rint ((item->y1 + origin - min) * item->canvas->pixels_per_unit);
		}

		/* OK, now fill the RGB buffer at x=i with a line between pymin and pymax,
		   or, if samples_per_unit == 1, then a dot at each location.
		*/

		if (pymax == pymin) {
			PAINT_DOTA(buf, waveview->wave_r, waveview->wave_g, waveview->wave_b, waveview->wave_a, x, pymin);
		} else {
			PAINT_VERTA(buf, waveview->wave_r, waveview->wave_g, waveview->wave_b, waveview->wave_a, x, pymax, pymin);
		}
		
		/* show clipped waveforms with small red lines */

		if (clip_max || clip_min) {
			clip_length = MIN(5,(waveview->height/4));
		}

		if (clip_max) {
			PAINT_VERTA(buf, waveview->clip_r, waveview->clip_g, waveview->clip_b, waveview->clip_a, x, pymax, pymax+clip_length);
		}

		if (clip_min) {
			PAINT_VERTA(buf, waveview->clip_r, waveview->clip_g, waveview->clip_b, waveview->clip_a, x, pymin-clip_length, pymin);
		}

		/* presto, we're done */
		
		cache_index++;
	}

	if (!waveview->rectified) {
		// Paint zeroline.
		//PAINT_HORIZA(buf, waveview->zero_r, waveview->zero_g, waveview->zero_b, waveview->zero_a, begin, endi-1, origin );
		
		unsigned char zero_r, zero_g, zero_b, zero_a;
		UINT_TO_RGBA( waveview->zero_color, &zero_r, &zero_g, &zero_b, &zero_a );
		int zeroline_y = (int) rint ((item->y1 + origin) * item->canvas->pixels_per_unit);
		PAINT_HORIZA(buf, zero_r, zero_g, zero_b, zero_a, zbegin, zend, zeroline_y);
	}
#undef origin

}

static void
gnome_canvas_waveview_draw (GnomeCanvasItem *item,
			  GdkDrawable *drawable,
			  int x, int y,
			  int width, int height)
{
	GnomeCanvasWaveView *waveview;

	waveview = GNOME_CANVAS_WAVEVIEW (item);

	if (parent_class->draw) {
		(* parent_class->draw) (item, drawable, x, y, width, height);
	}

	fprintf (stderr, "please don't use the CanvasWaveView item in a non-aa Canvas\n");
	abort ();
}

static void
gnome_canvas_waveview_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasWaveView *waveview = GNOME_CANVAS_WAVEVIEW (item);

	*x1 = waveview->x;
	*y1 = waveview->y;

	*x2 = ceil (*x1 + (waveview->length_function (waveview->data_src) / waveview->samples_per_unit));
	*y2 = *y1 + waveview->height;

#if 0
	x = 0; y = 0;
	gnome_canvas_item_i2w (item, &x, &y);
	gnome_canvas_w2c_d (GNOME_CANVAS(item->canvas), x, y, &a, &b);
	x = *x2;
	y = *y2;
	gnome_canvas_item_i2w (item, &x, &y);
	gnome_canvas_w2c_d (GNOME_CANVAS(item->canvas), x, y, &c, &d);
	printf ("item bounds now (%g,%g),(%g,%g)\n", a, b, c, d);
#endif		

}

static double
gnome_canvas_waveview_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	/* XXX for now, point is never inside the wave 
	GnomeCanvasWaveView *waveview;
	double x1, y1, x2, y2;
	double dx, dy;
	*/

	return DBL_MAX;

#if 0
	waveview = GNOME_CANVAS_WAVEVIEW (item);

	*actual_item = item;

	/* Find the bounds for the rectangle plus its outline width */

	gnome_canvas_waveview_bounds (item, &x1, &y1, &x2, &y2);

	/* Is point inside rectangle */
	
	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {
		return 0.0;
	}

	/* Point is outside rectangle */

	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
#endif
}

