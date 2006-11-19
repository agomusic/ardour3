/* IO connection */

CONFIG_VARIABLE (AutoConnectOption, output_auto_connect, "output-auto-connect", AutoConnectOption (0))
CONFIG_VARIABLE (AutoConnectOption, input_auto_connect, "input-auto-connect", AutoConnectOption (0))

#ifdef __APPLE__
CONFIG_VARIABLE (std::string, auditioner_output_left, "auditioner-output-left",  "coreaudio:Built-in Audio:in1")
CONFIG_VARIABLE (std::string, auditioner_output_right, "auditioner-output-right", "coreaudio:Built-in Audio:in2")
#else
CONFIG_VARIABLE (std::string, auditioner_output_left, "auditioner-output-left", "alsa_pcm:playback_1")
CONFIG_VARIABLE (std::string, auditioner_output_right, "auditioner-output-right", "alsa_pcm:playback_2")
#endif

/* MIDI and MIDI related */

CONFIG_VARIABLE (std::string, mtc_port_name, "mtc-port-name", "default")
CONFIG_VARIABLE (std::string, mmc_port_name, "mmc-port-name", "default")
CONFIG_VARIABLE (std::string, midi_port_name, "midi-port-name", "default")
CONFIG_VARIABLE (bool, trace_midi_input, "trace-midi-input", false)
CONFIG_VARIABLE (bool, trace_midi_output, "trace-midi-output", false)
CONFIG_VARIABLE (bool, send_mtc, "send-mtc", false)
CONFIG_VARIABLE (bool, send_mmc, "send-mmc", false)
CONFIG_VARIABLE (bool, mmc_control, "mmc-control", false)
CONFIG_VARIABLE (bool, midi_feedback, "midi-feedback", false)
CONFIG_VARIABLE (bool, midi_control, "midi-control", false)

/* control surfaces */

CONFIG_VARIABLE (uint32_t, feedback_interval_ms,  "feedback-interval-ms", 100)
CONFIG_VARIABLE (bool, use_tranzport,  "use-tranzport", false)

/* disk operations */

CONFIG_VARIABLE (uint32_t, minimum_disk_io_bytes,  "minimum-disk-io-bytes", 1024 * 256)
CONFIG_VARIABLE (float, track_buffer_seconds, "track-buffer-seconds", 5.0)
CONFIG_VARIABLE (uint32_t, disk_choice_space_threshold,  "disk-choice-space-threshold", 57600000)
CONFIG_VARIABLE (SampleFormat, native_file_data_format,  "native-file-data-format", ARDOUR::FormatFloat)
CONFIG_VARIABLE (HeaderFormat, native_file_header_format,  "native-file-header-format", ARDOUR::WAVE)

/* OSC */

CONFIG_VARIABLE (uint32_t, osc_port, "osc-port", 3819)
CONFIG_VARIABLE (bool, use_osc, "use-osc", true)
CONFIG_VARIABLE (bool, use_overlap_equivalency, "use-overlap-equivalency", true)

/* crossfades */

CONFIG_VARIABLE (CrossfadeModel, xfade_model, "xfade-model", FullCrossfade)
CONFIG_VARIABLE (bool, auto_xfade, "auto-xfade", true)
CONFIG_VARIABLE (float, short_xfade_seconds, "short-xfade-seconds", 0.015)
CONFIG_VARIABLE (bool, xfades_active, "xfades-active", true)
CONFIG_VARIABLE (bool, xfades_visible, "xfades-visible", true)
CONFIG_VARIABLE (uint32_t, destructive_xfade_msecs,  "destructive-xfade-msecs", 2)

/* editing related */

CONFIG_VARIABLE (EditMode, edit_mode, "edit-mode", Slide)
CONFIG_VARIABLE (LayerModel, layer_model, "layer-model", MoveAddHigher)

/* monitoring, mute, solo etc */

CONFIG_VARIABLE (bool, mute_affects_pre_fader, "mute-affects-pre-fader", true)
CONFIG_VARIABLE (bool, mute_affects_post_fader, "mute-affects-post-fader", true)
CONFIG_VARIABLE (bool, mute_affects_control_outs, "mute-affects-control-outs", true)
CONFIG_VARIABLE (bool, mute_affects_main_outs, "mute-affects-main-outs", true)
CONFIG_VARIABLE (MonitorModel, monitoring_model, "monitoring-model", SoftwareMonitoring)
CONFIG_VARIABLE (SoloModel, solo_model, "solo-model", InverseMute)
CONFIG_VARIABLE (bool, solo_latched, "solo-latched", true)
CONFIG_VARIABLE (bool, latched_record_enable, "latched-record-enable", false)
CONFIG_VARIABLE (bool, all_safe, "all-safe", false)

/* click */

CONFIG_VARIABLE (bool, clicking, "clicking", false)
CONFIG_VARIABLE (std::string, click_sound, "click-sound", "")
CONFIG_VARIABLE (std::string, click_emphasis_sound, "click-emphasis-sound", "")

/* transport control and related */

CONFIG_VARIABLE (bool, auto_play, "auto-play", false)
CONFIG_VARIABLE (bool, auto_return, "auto-return", false)
CONFIG_VARIABLE (bool, auto_input, "auto-input", true)
CONFIG_VARIABLE (bool, punch_in, "punch-in", false)
CONFIG_VARIABLE (bool, punch_out, "punch-out", false)
CONFIG_VARIABLE (bool, plugins_stop_with_transport, "plugins-stop-with-transport", false)
CONFIG_VARIABLE (bool, do_not_record_plugins, "do-not-record-plugins", false)
CONFIG_VARIABLE (bool, stop_recording_on_xrun, "stop-recording-on-xrun", false)
CONFIG_VARIABLE (bool, stop_at_session_end, "stop-at-session-end", true)
CONFIG_VARIABLE (bool, seamless_loop, "seamless-loop", false)
CONFIG_VARIABLE (nframes_t, preroll, "preroll", 0)
CONFIG_VARIABLE (nframes_t, postroll, "postroll", 0)
CONFIG_VARIABLE (float, rf_speed, "rf-speed", 2.0f)
CONFIG_VARIABLE (float, shuttle_speed_factor, "shuttle-speed-factor", 1.0f)
CONFIG_VARIABLE (float, shuttle_speed_threshold, "shuttle-speed-threshold", 5.0f)
CONFIG_VARIABLE (SlaveSource, slave_source, "slave-source", None)
CONFIG_VARIABLE (ShuttleBehaviour, shuttle_behaviour, "shuttle-behaviour", Sprung)
CONFIG_VARIABLE (ShuttleUnits, shuttle_units, "shuttle-units", Percentage)
CONFIG_VARIABLE (bool, quieten_at_speed, "quieten-at-speed", true)

/* timecode and sync */

CONFIG_VARIABLE (bool, jack_time_master, "jack-time-master", true)
CONFIG_VARIABLE (bool, use_video_sync, "use-video-sync", false)
CONFIG_VARIABLE (bool, timecode_source_is_synced, "timecode-source-is-synced", true)
CONFIG_VARIABLE (float, smpte_frames_per_second, "smpte-frames-per-second", 30.0f)
CONFIG_VARIABLE (float, video_pullup, "video-pullup", 0.0f)
CONFIG_VARIABLE (bool,  smpte_drop_frames, "smpte-drop-frames", false)

/* metering */

CONFIG_VARIABLE (float, meter_hold, "meter-hold", 100.0f)
CONFIG_VARIABLE (float, meter_falloff, "meter-falloff", 0.375f)
CONFIG_VARIABLE (nframes_t, over_length_short, "over-length-short", 2)
CONFIG_VARIABLE (nframes_t, over_length_long, "over-length-long", 10)

/* miscellany */
	
CONFIG_VARIABLE (bool, hiding_groups_deactivates_groups, "hiding-groups-deactivates-groups", true)
CONFIG_VARIABLE (bool, verify_remove_last_capture, "verify-remove-last-capture", true)
CONFIG_VARIABLE (bool, no_new_session_dialog, "no-new-session-dialog", false)
CONFIG_VARIABLE (bool, use_vst, "use-vst", true)

/* BWAV */

CONFIG_VARIABLE (string, bwf_country_code, "bwf-country-code", "US")
CONFIG_VARIABLE (string, bwf_organization_code, "bwf-organization-code", "US")

/* these variables have custom set() methods (e.g. path globbing) */

CONFIG_VARIABLE_SPECIAL(std::string, raid_path, "raid-path", "", path_expand)
