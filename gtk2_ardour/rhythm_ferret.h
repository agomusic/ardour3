#ifndef __gtk2_ardour_rhythm_ferret_h__
#define __gtk2_ardour_rhythm_ferret_h__

#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/frame.h>
#include <gtkmm/image.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>

#include "ardour_dialog.h"

namespace ARDOUR {
	class Readable;
}

class PublicEditor;
class RegionView;

class RhythmFerret : public ArdourDialog {
  public:
	/* order of these enums must match the _analyse_mode_strings
	   in rhythm_ferret.cc
	*/
	enum AnalysisMode {
		PercussionOnset,
		NoteOnset
	};

	enum Action {
		SplitRegion,
		DefineTempoMap,
		ConformRegion
	};
	
	RhythmFerret (PublicEditor&);
	~RhythmFerret ();

	void set_session (ARDOUR::Session*);
	
  private:
	PublicEditor& editor;

	Gtk::HBox  upper_hpacker;
	Gtk::HBox  lower_hpacker;

	Gtk::Frame operation_frame;
	Gtk::Frame selection_frame;
	Gtk::Frame ferret_frame;

	Gtk::VBox  op_logo_packer;
	Gtk::Image* logo;

	/* operation frame */

	Gtk::VBox op_packer;
	Gtk::RadioButtonGroup operation_button_group;
	Gtk::RadioButton region_split_button;
	Gtk::RadioButton tempo_button;
	Gtk::RadioButton region_conform_button;

	/* analysis frame */

	Gtk::VBox ferret_packer;
	Gtk::ComboBoxText analysis_mode_selector;
	Gtk::Label analysis_mode_label;
	Gtk::Adjustment detection_threshold_adjustment;
	Gtk::HScale detection_threshold_scale;
	Gtk::Label detection_threshold_label;
	Gtk::Adjustment sensitivity_adjustment;
	Gtk::HScale sensitivity_scale;
	Gtk::Label sensitivity_label;
	Gtk::Button analyze_button;
	Gtk::Adjustment trigger_gap_adjustment;
	Gtk::SpinButton trigger_gap_spinner;
	Gtk::Label trigger_gap_label;

	Gtk::Label operation_clarification_label;
	Gtk::Button action_button;

	std::vector<std::string> analysis_mode_strings;

	std::vector<nframes64_t> current_results;

	AnalysisMode get_analysis_mode () const;
	Action get_action() const;

	void run_analysis ();
	int run_percussion_onset_analysis (boost::shared_ptr<ARDOUR::Readable> region, nframes64_t offset, std::vector<nframes64_t>& results);

	void do_action ();
	void do_split_action ();
	void do_region_split (RegionView* rv, const std::vector<nframes64_t>&);
};

#endif /* __gtk2_ardour_rhythm_ferret_h__ */
