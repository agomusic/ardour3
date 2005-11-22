#include "simplerect.h"
#include "waveview.h"
#include "ghostregion.h"
#include "automation_time_axis.h"
#include "rgb_macros.h"

using namespace Editing;
using namespace ArdourCanvas;

GhostRegion::GhostRegion (AutomationTimeAxisView& atv, double initial_pos)
	: trackview (atv)
{
  //group = gnome_canvas_item_new (GNOME_CANVAS_GROUP(trackview.canvas_display),
  //			     gnome_canvas_group_get_type(),
  //			     "x", initial_pos,
  //			     "y", 0.0,
  //			     NULL);
	group = new Gnome::Canvas::Group (*trackview.canvas_display);
	group->set_property ("x", initial_pos);
	group->set_property ("y", 0.0);

	base_rect = new Gnome::Canvas::SimpleRect (*group);
	base_rect->set_property ("x1", (double) 0.0);
	base_rect->set_property ("y1", (double) 0.0);
	base_rect->set_property ("y2", (double) trackview.height);
	base_rect->set_property ("outline_what", (guint32) 0);
	base_rect->set_property ("outline_color_rgba", color_map[cGhostTrackBaseOutline]);
	base_rect->set_property ("fill_color_rgba", color_map[cGhostTrackBaseFill]);
	group->lower_to_bottom ();

	atv.add_ghost (this);
}

GhostRegion::~GhostRegion ()
{
	GoingAway (this);
	delete base_rect;
	delete group;
}

void
GhostRegion::set_samples_per_unit (double spu)
{
	for (vector<WaveView*>::iterator i = waves.begin(); i != waves.end(); ++i) {
		(*i)->property_samples_per_unit().set_value(spu);
	}		
}

void
GhostRegion::set_duration (double units)
{
        base_rect->set_property ("x2", units);
}

void
GhostRegion::set_height ()
{
	gdouble ht;
	vector<WaveView*>::iterator i;
	uint32_t n;

	base_rect->set_property ("y2", (double) trackview.height);
	ht = ((trackview.height) / (double) waves.size());
	
	for (n = 0, i = waves.begin(); i != waves.end(); ++i, ++n) {
		gdouble yoff = n * ht;
		(*i)->property_height().set_value(ht);
		(*i)->property_y().set_value(yoff);
	}
}

