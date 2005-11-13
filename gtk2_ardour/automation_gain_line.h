#ifndef __ardour_gtk_automation_gain_line_h__
#define __ardour_gtk_automation_gain_line_h__

#include <ardour/ardour.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include <gtkmm.h>

#include "automation_line.h"

namespace ARDOUR {
	class Session;
}


class TimeAxisView;

class AutomationGainLine : public AutomationLine
{
  public:
  AutomationGainLine (string name, ARDOUR::Session&, TimeAxisView&, Gnome::Canvas::Group& parent,
		      ARDOUR::Curve&, 
		      sigc::slot<bool,GdkEvent*,ControlPoint*>, sigc::slot<bool,GdkEvent*,AutomationLine*>);
	
	void view_to_model_y (double&);
	void model_to_view_y (double&);

  private:
	ARDOUR::Session& session;

};


#endif /* __ardour_gtk_automation_gain_line_h__ */


