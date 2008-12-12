#ifndef  __gtk_ardour_port_group_h__ 
#define  __gtk_ardour_port_group_h__ 

#include <vector>
#include <string>

#include <gtkmm/widget.h>
#include <gtkmm/checkbutton.h>

#include <ardour/data_type.h>

namespace ARDOUR {
	class Session;
	class IO;
	class PortInsert;
}

class PortMatrix;

/// A list of port names, grouped by some aspect of their type e.g. busses, tracks, system
class PortGroup
{
  public:
	/** PortGroup constructor.
	 * @param n Name.
	 * @param p Port name prefix.
	 * @param v true if group should be visible in the UI, otherwise false.
	 */
	PortGroup (std::string const & n, std::string const & p, bool v) : name (n), prefix (p), visible (v) {}

	void add (std::string const & p);

	std::string name; ///< name for the group
	std::string prefix; ///< prefix (before colon) e.g. "ardour:"
	std::vector<std::string> ports; ///< port names
	bool visible; ///< true if the group is visible in the UI
};

/// The UI for a PortGroup
class PortGroupUI
{
  public:
	PortGroupUI (PortMatrix&, PortGroup&);

	Gtk::Widget& get_visibility_checkbutton ();
	PortGroup& port_group () { return _port_group; }
	void setup_visibility ();

  private:
	void port_checkbutton_toggled (Gtk::CheckButton*, int, int);
	bool port_checkbutton_release (GdkEventButton* ev, Gtk::CheckButton* b, int r, int c);
	void visibility_checkbutton_toggled ();

	PortMatrix& _port_matrix; ///< the PortMatrix that we are working for
	PortGroup& _port_group; ///< the PortGroup that we are representing
	bool _ignore_check_button_toggle;
	Gtk::CheckButton _visibility_checkbutton;
};

/// A list of PortGroups
class PortGroupList : public std::list<PortGroup*>
{
  public:
	enum Mask {
		BUSS = 0x1,
		TRACK = 0x2,
		SYSTEM = 0x4,
		OTHER = 0x8
	};

	PortGroupList (ARDOUR::Session &, ARDOUR::DataType, bool, Mask);

	void refresh ();
	int n_visible_ports () const;
	std::string get_port_by_index (int, bool with_prefix = true) const;
	void set_type (ARDOUR::DataType);
	void set_offer_inputs (bool);
	
  private:
	ARDOUR::Session& _session;
	ARDOUR::DataType _type;
	bool _offer_inputs;

	PortGroup buss;
	PortGroup track;
	PortGroup system;
	PortGroup other;
};

#endif /* __gtk_ardour_port_group_h__ */