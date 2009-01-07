/*
    Copyright (C) 2000-2007 Paul Davis 

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

*/

#ifndef __gtk_ardour_public_editor_h__
#define __gtk_ardour_public_editor_h__

#include <map>

#include <string>
#include <glib.h>
#include <gdk/gdktypes.h>
#include <gtkmm/window.h>
#include <gtkmm/actiongroup.h>
#include <jack/types.h>
#include <sigc++/signal.h>

#include <pbd/statefuldestructible.h> 

#include "editing.h"
#include "canvas.h"
#include "selection.h"

namespace ARDOUR {
	class Session;
	class Region;
	class Playlist;
	class RouteGroup;
}

namespace Gtk {
	class Container;
	class Menu;
}

class Editor;
class TimeAxisViewItem;
class TimeAxisView;
class PluginUIWindow;
class PluginSelector;
class PlaylistSelector;
class XMLNode;
class Selection;
class AutomationLine;
class ControlPoint;
class SelectionRect;
class CrossfadeView;
class RouteTimeAxisView;
class RegionView;
class AudioRegionView;
class TempoMarker;
class MeterMarker;
class Marker;
class AutomationTimeAxisView;
class MarkerTimeAxis;
class ImageFrameView;
class ImageFrameTimeAxis;
class MarkerView;

/// Representation of the interface of the Editor class

/** This class contains just the public interface of the Editor class,
 * in order to decouple it from the private implementation, so that callers
 * of PublicEditor need not be recompiled if private methods or member variables
 * change.
 */
class PublicEditor : public Gtk::Window, public PBD::StatefulThingWithGoingAway {
  public:
	PublicEditor ();
	virtual ~PublicEditor ();

	typedef TrackSelection TrackViewList;

	/** @return Singleton PublicEditor instance */
	static PublicEditor& instance () { return *_instance; }

	virtual bool have_idled() const = 0;
	virtual void first_idle() = 0;

	/** Attach this editor to a Session.
	 * @param s Session to connect to.
	 */
	virtual void connect_to_session (ARDOUR::Session* s) = 0;

	/** @return The Session that we are editing, or 0 */
	virtual ARDOUR::Session* current_session () const = 0;

	/** Set the snap type.
	 * @param t Snap type (defined in editing_syms.h)
	 */
	virtual void set_snap_to (Editing::SnapType t) = 0;

	/** Set the snap mode.
	 * @param m Snap mode (defined in editing_syms.h)
	 */
	virtual void set_snap_mode (Editing::SnapMode m) = 0;

	/** Set the snap threshold.
	 * @param t Snap threshold in `units'.
	 */
	virtual void set_snap_threshold (double t) = 0;
	
	/** Snap a value according to the current snap setting. */
	virtual void snap_to (nframes64_t& first, int32_t direction = 0, bool for_mark = false) = 0;
	
	/** Get the current snap value in beats */
	virtual double snap_length_beats (nframes64_t start) = 0;

	/** Undo some transactions.
	 * @param n Number of transactions to undo.
	 */

	virtual void undo (uint32_t n = 1) = 0;

	/** Redo some transactions.
	 * @param n Number of transaction to redo.
	 */
	virtual void redo (uint32_t n = 1) = 0;

	/** Set the mouse mode (gain, object, range, timefx etc.)
	 * @param m Mouse mode (defined in editing_syms.h)
	 * @param force Perform the effects of the change even if no change is required
	 * (ie even if the current mouse mode is equal to \ref m)
	 */
	virtual void set_mouse_mode (Editing::MouseMode m, bool force = false) = 0;

	/** Step the mouse mode onto the next or previous one.
	 * @param next true to move to the next, otherwise move to the previous
	 */
	virtual void step_mouse_mode (bool next) = 0;

	/** @return The current mouse mode (gain, object, range, timefx etc.)
	 * (defined in editing_syms.h)
	 */
	virtual Editing::MouseMode current_mouse_mode () const = 0;
	
	/** Set the midi edit mode (pencil, select, eraser, etc.)
	 * @param m Midi edit mode (defined in editing_syms.h)
	 * @param force Perform the effects of the change even if no change is required
	 * (ie even if the current midi edit mode is equal to \ref m)
	 */
	virtual void set_midi_edit_mode (Editing::MidiEditMode m, bool force = false) = 0;
	
	/** @return The current mouse mode (gain, object, range, timefx etc.)
	 * (defined in editing_syms.h)
	 */
	virtual Editing::MidiEditMode current_midi_edit_mode () const = 0;
	
	/** @return Sound edited notes in MIDI regions while editing
	 */
	virtual bool sound_notes () const = 0;

	/** Possibly start the audition of a region.  If \ref r is 0, or not an AudioRegion
	 * any current audition is cancelled.  If we are currently auditioning \ref r,
	 * the audition will be cancelled.  Otherwise an audition of \ref r will start.
	 * \param r Region to consider.
	 */
	virtual void consider_auditioning (boost::shared_ptr<ARDOUR::Region> r) = 0;

	/** Set whether waveforms should be shown for audio tracks.
	 * @param yn true to show waveforms, otherwise false.
	 */
	virtual void set_show_waveforms (bool yn) = 0;

	/** @return true if waveforms are being shown, otherwise false */
	virtual bool show_waveforms () const = 0;

	/** Set whether waveforms should be shown while recording audio tracks.
	 * @param yn true to show waveforms, otherwise false.
	 */
	virtual void set_show_waveforms_recording (bool yn) = 0;

	/** @return true if waveforms are being shown while recording, otherwise false */
	virtual bool show_waveforms_recording () const = 0;
	virtual void new_region_from_selection () = 0;
	virtual void separate_region_from_selection () = 0;

	virtual void toggle_playback (bool with_abort) = 0;
	virtual void transition_to_rolling (bool fwd) = 0;
	virtual nframes64_t unit_to_frame (double unit) const = 0;
	// XXX remove me when libardour goes nframes64_t
	double frame_to_unit (nframes_t frame) const { 
		return frame_to_unit ((nframes64_t) frame);
	}
	virtual double frame_to_unit (nframes64_t frame) const = 0;
	virtual double frame_to_unit (double frame) const = 0;
	virtual nframes64_t pixel_to_frame (double pixel) const = 0;
	virtual gulong frame_to_pixel (nframes64_t frame) const = 0;
	virtual Selection& get_selection () const = 0;
	virtual Selection& get_cut_buffer () const = 0;
	virtual bool extend_selection_to_track (TimeAxisView&) = 0;
	virtual void play_selection () = 0;
	virtual void set_show_measures (bool yn) = 0;
	virtual bool show_measures () const = 0;
	virtual bool redraw_measures () = 0;

	/** Open main export dialog */
	virtual void export_audio () = 0;
	
	/** Open export dialog with current selection pre-selected */
	virtual void export_selection () = 0;
	
	/** Open export dialog with current range pre-selected */
	virtual void export_range () = 0;

	virtual void register_actions () = 0;
	virtual void add_toplevel_controls (Gtk::Container&) = 0;
	virtual void set_zoom_focus (Editing::ZoomFocus) = 0;
	virtual Editing::ZoomFocus get_zoom_focus () const = 0;
	virtual gdouble   get_current_zoom () const = 0;
	virtual PlaylistSelector& playlist_selector() const = 0;
	virtual void route_name_changed (TimeAxisView *) = 0;
	virtual void clear_playlist (boost::shared_ptr<ARDOUR::Playlist>) = 0;
	virtual void new_playlists (TimeAxisView*) = 0;
	virtual void copy_playlists (TimeAxisView*) = 0;
	virtual void clear_playlists (TimeAxisView*) = 0;
	virtual void select_all_tracks () = 0;
	virtual void set_selected_track (TimeAxisView&, Selection::Operation op = Selection::Set, bool no_remove = false) = 0;
	virtual void set_selected_mixer_strip (TimeAxisView&) = 0;
	virtual void hide_track_in_display (TimeAxisView& tv, bool temporary = false) = 0;
	virtual void show_track_in_display (TimeAxisView& tv) = 0;

	/** Set whether the editor should follow the playhead.
	 * @param yn true to follow playhead, otherwise false.
	 */
	virtual void set_follow_playhead (bool yn) = 0;

	/** Toggle whether the editor is following the playhead */
	virtual void toggle_follow_playhead () = 0;

	/** @return true if the editor is following the playhead */
	virtual bool follow_playhead () const = 0;

	/** @return true if the playhead is currently being dragged, otherwise false */
	virtual bool dragging_playhead () const = 0;
	virtual double get_physical_screen_width() const = 0;
	virtual void ensure_float (Gtk::Window&) = 0;
	virtual void show_window () = 0;
	virtual TrackViewList* get_valid_views (TimeAxisView*, ARDOUR::RouteGroup* grp = 0) = 0;
	virtual nframes64_t leftmost_position() const = 0;
	virtual nframes64_t current_page_frames() const = 0;
	virtual void temporal_zoom_step (bool coarser) = 0;
	virtual void scroll_tracks_down_line () = 0;
	virtual void scroll_tracks_up_line () = 0;
	virtual bool new_regionviews_display_gain () = 0;
	virtual void prepare_for_cleanup () = 0;
	virtual void finish_cleanup () = 0;
	virtual void reset_x_origin (nframes64_t frame) = 0;
	virtual void remove_last_capture () = 0;
	virtual void maximise_editing_space () = 0;
	virtual void restore_editing_space () = 0;
	virtual nframes64_t get_preferred_edit_position (bool ignore_playhead = false) = 0;
	virtual void toggle_meter_updating() = 0;
	virtual void split_region_at_points (boost::shared_ptr<ARDOUR::Region>, ARDOUR::AnalysisFeatureList&, bool can_ferret) = 0;
	virtual void mouse_add_new_marker (nframes64_t where, bool is_cd=false, bool is_xrun=false) = 0;
	virtual void foreach_time_axis_view (sigc::slot<void,TimeAxisView&>) = 0;
	virtual void add_to_idle_resize (TimeAxisView*,uint32_t) = 0;

#ifdef WITH_CMT
	virtual void add_imageframe_time_axis(const std::string & track_name, void*)  = 0;
	virtual void add_imageframe_marker_time_axis(const std::string & track_name, TimeAxisView* marked_track, void*)  = 0;
	virtual void connect_to_image_compositor()  = 0;
	virtual void scroll_timeaxis_to_imageframe_item(const TimeAxisViewItem* item)  = 0;
	virtual TimeAxisView* get_named_time_axis(const std::string & name)  = 0;
#endif

	virtual RouteTimeAxisView* get_route_view_by_id (PBD::ID& id) = 0;

	virtual void get_equivalent_regions (RegionView* rv, std::vector<RegionView*>&) const = 0;

	sigc::signal<void> ZoomFocusChanged;
	sigc::signal<void> ZoomChanged;
	sigc::signal<void> Resized;
	sigc::signal<void> Realized;
	sigc::signal<void,nframes64_t> UpdateAllTransportClocks;

	Glib::RefPtr<Gtk::ActionGroup> editor_actions;

	virtual void reset_focus () = 0;

	virtual bool canvas_control_point_event (GdkEvent* event, ArdourCanvas::Item*, ControlPoint*) = 0;
	virtual bool canvas_line_event (GdkEvent* event, ArdourCanvas::Item*, AutomationLine*) = 0;
	virtual bool canvas_selection_rect_event (GdkEvent* event, ArdourCanvas::Item*, SelectionRect*) = 0;
	virtual bool canvas_selection_start_trim_event (GdkEvent* event, ArdourCanvas::Item*, SelectionRect*) = 0;
	virtual bool canvas_selection_end_trim_event (GdkEvent* event, ArdourCanvas::Item*, SelectionRect*) = 0;
	virtual bool canvas_crossfade_view_event (GdkEvent* event, ArdourCanvas::Item*, CrossfadeView*) = 0;
	virtual bool canvas_fade_in_event (GdkEvent* event, ArdourCanvas::Item*, AudioRegionView*) = 0;
	virtual bool canvas_fade_in_handle_event (GdkEvent* event, ArdourCanvas::Item*, AudioRegionView*) = 0;
	virtual bool canvas_fade_out_event (GdkEvent* event, ArdourCanvas::Item*, AudioRegionView*) = 0;
	virtual bool canvas_fade_out_handle_event (GdkEvent* event, ArdourCanvas::Item*, AudioRegionView*) = 0;
	virtual bool canvas_region_view_event (GdkEvent* event, ArdourCanvas::Item*, RegionView*) = 0;
	virtual bool canvas_region_view_name_highlight_event (GdkEvent* event, ArdourCanvas::Item*, RegionView*) = 0;
	virtual bool canvas_region_view_name_event (GdkEvent* event, ArdourCanvas::Item*, RegionView*) = 0;
	virtual bool canvas_stream_view_event (GdkEvent* event, ArdourCanvas::Item*, RouteTimeAxisView*) = 0;
	virtual bool canvas_marker_event (GdkEvent* event, ArdourCanvas::Item*, Marker*) = 0;
	virtual bool canvas_zoom_rect_event (GdkEvent* event, ArdourCanvas::Item*) = 0;
	virtual bool canvas_tempo_marker_event (GdkEvent* event, ArdourCanvas::Item*, TempoMarker*) = 0;
	virtual bool canvas_meter_marker_event (GdkEvent* event, ArdourCanvas::Item*, MeterMarker*) = 0;
	virtual bool canvas_automation_track_event(GdkEvent* event, ArdourCanvas::Item*, AutomationTimeAxisView*) = 0;

	virtual bool canvas_tempo_bar_event (GdkEvent* event, ArdourCanvas::Item*) = 0;
	virtual bool canvas_meter_bar_event (GdkEvent* event, ArdourCanvas::Item*) = 0;
	virtual bool canvas_marker_bar_event (GdkEvent* event, ArdourCanvas::Item*) = 0;
	virtual bool canvas_range_marker_bar_event (GdkEvent* event, ArdourCanvas::Item*) = 0;
	virtual bool canvas_transport_marker_bar_event (GdkEvent* event, ArdourCanvas::Item*) = 0;

#ifdef WITH_CMT
	virtual bool canvas_imageframe_item_view_event(GdkEvent* event, ArdourCanvas::Item*,ImageFrameView*) = 0;
	virtual bool canvas_imageframe_view_event(GdkEvent* event, ArdourCanvas::Item*,ImageFrameTimeAxis*) = 0;
	virtual bool canvas_imageframe_start_handle_event(GdkEvent* event, ArdourCanvas::Item*,ImageFrameView*) = 0;
	virtual bool canvas_imageframe_end_handle_event(GdkEvent* event, ArdourCanvas::Item*,ImageFrameView*) = 0;
	virtual bool canvas_marker_time_axis_view_event(GdkEvent* event, ArdourCanvas::Item*,MarkerTimeAxis*) = 0;
	virtual bool canvas_markerview_item_view_event(GdkEvent* event, ArdourCanvas::Item*,MarkerView*) = 0;
	virtual bool canvas_markerview_start_handle_event(GdkEvent* event, ArdourCanvas::Item*,MarkerView*) = 0;
	virtual bool canvas_markerview_end_handle_event(GdkEvent* event, ArdourCanvas::Item*,MarkerView*) = 0;
#endif

	virtual void queue_draw_resize_line (int at) = 0;
	virtual void start_resize_line_ops () = 0;
	virtual void end_resize_line_ops () = 0;

	static const int window_border_width;
	static const int container_border_width;
	static const int vertical_spacing;
	static const int horizontal_spacing;


	virtual gdouble get_trackview_group_vertical_offset () const = 0;
	virtual gdouble get_canvas_timebars_vsize () const = 0;
	virtual ArdourCanvas::Group* get_background_group () const = 0;
	virtual ArdourCanvas::Group* get_trackview_group () const = 0;

	/// Singleton instance, set up by Editor::Editor()

	static PublicEditor* _instance;

	friend class PluginUIWindow;
};

#endif // __gtk_ardour_public_editor_h__
