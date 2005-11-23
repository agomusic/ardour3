#ifndef __ardour_gtk_region_gain_line_h__
#define __ardour_gtk_region_gain_line_h__

#include <ardour/ardour.h>
#include <libgnomecanvasmm/libgnomecanvasmm.h>
#include <gtkmm.h>

#include "automation_line.h"

namespace ARDOUR {
	class Session;
}

class TimeAxisView;
class AudioRegionView;

class AudioRegionGainLine : public AutomationLine
{
  public:
  AudioRegionGainLine (string name, ARDOUR::Session&, AudioRegionView&, ArdourCanvas::Group& parent,
			     ARDOUR::Curve&, 
		       bool (*point_callback)(ArdourCanvas::Item*, GdkEvent*, gpointer),
		       bool (*line_callback)(ArdourCanvas::Item*, GdkEvent*, gpointer));
	
	void view_to_model_y (double&);
	void model_to_view_y (double&);

	void start_drag (ControlPoint*, float fraction);
	void end_drag (ControlPoint*);

	void remove_point (ControlPoint&);



  private:
	ARDOUR::Session& session;
	AudioRegionView& rv;

	UndoAction get_memento();
};


#endif /* __ardour_gtk_region_gain_line_h__ */
