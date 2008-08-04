/*
    Copyright (C) 2000-2003 Paul Davis

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

#define __STDC_FORMAT_MACROS 1
#include <stdint.h>

#include <cstdio> /* for sprintf */
#include <cstring>
#include <cmath>
#include <cctype>
#include <string>
#include <cerrno>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_WORDEXP
#include <wordexp.h>
#endif

#include <pbd/error.h>
#include <pbd/stacktrace.h>
#include <pbd/xml++.h>
#include <pbd/basename.h>
#include <ardour/utils.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace std;
using namespace PBD;
using Glib::ustring;

ustring
legalize_for_path (ustring str)
{
	ustring::size_type pos;
	ustring legal_chars = "abcdefghijklmnopqrtsuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_+=: ";
	ustring legal;

	legal = str;
	pos = 0;

	while ((pos = legal.find_first_not_of (legal_chars, pos)) != string::npos) {
		legal.replace (pos, 1, "_");
		pos += 1;
	}

	return legal;
}

string bump_name_once(std::string name)
{
	string::size_type period;
	string newname;

	if ((period = name.find_last_of ('.')) == string::npos) {
		newname  = name;
		newname += ".1";
	} else {
		int isnumber = 1;
		const char *last_element = name.c_str() + period + 1;
		for (size_t i = 0; i < strlen(last_element); i++) {
			if (!isdigit(last_element[i])) {
				isnumber = 0;
				break;
			}
		}

		errno = 0;
		long int version = strtol (name.c_str()+period+1, (char **)NULL, 10);

		if (isnumber == 0 || errno != 0) {
			// last_element is not a number, or is too large
			newname  = name;
			newname += ".1";
		} else {
			char buf[32];

			snprintf (buf, sizeof(buf), "%ld", version+1);

			newname  = name.substr (0, period+1);
			newname += buf;
		}
	}

	return newname;

}

ostream&
operator<< (ostream& o, const BBT_Time& bbt)
{
	o << bbt.bars << '|' << bbt.beats << '|' << bbt.ticks;
	return o;
}

XMLNode *
find_named_node (const XMLNode& node, string name)
{
	XMLNodeList nlist;
	XMLNodeConstIterator niter;
	XMLNode* child;

	nlist = node.children();

	for (niter = nlist.begin(); niter != nlist.end(); ++niter) {

		child = *niter;

		if (child->name() == name) {
			return child;
		}
	}

	return 0;
}

int
cmp_nocase (const string& s, const string& s2)
{
	string::const_iterator p = s.begin();
	string::const_iterator p2 = s2.begin();

	while (p != s.end() && p2 != s2.end()) {
		if (toupper(*p) != toupper(*p2)) {
			return (toupper(*p) < toupper(*p2)) ? -1 : 1;
		}
		++p;
		++p2;
	}

	return (s2.size() == s.size()) ? 0 : (s.size() < s2.size()) ? -1 : 1;
}

int
touch_file (ustring path)
{
	int fd = open (path.c_str(), O_RDWR|O_CREAT, 0660);
	if (fd >= 0) {
		close (fd);
		return 0;
	}
	return 1;
}

ustring
region_name_from_path (ustring path, bool strip_channels, bool add_channel_suffix, uint32_t total, uint32_t this_one)
{
	path = PBD::basename_nosuffix (path);

	if (strip_channels) {

		/* remove any "?R", "?L" or "?[a-z]" channel identifier */

		ustring::size_type len = path.length();

		if (len > 3 && (path[len-2] == '%' || path[len-2] == '?' || path[len-2] == '.') &&
		    (path[len-1] == 'R' || path[len-1] == 'L' || (islower (path[len-1])))) {

			path = path.substr (0, path.length() - 2);
		}
	}

	if (add_channel_suffix) {

		path += '%';

		if (total > 2) {
			path += (char) ('a' + this_one);
		} else {
			path += (char) (this_one == 0 ? 'L' : 'R');
		}
	}

	return path;
}

bool
path_is_paired (ustring path, ustring& pair_base)
{
	ustring::size_type pos;

	/* remove any leading path */

	if ((pos = path.find_last_of ('/')) != string::npos) {
		path = path.substr(pos+1);
	}

 	/* remove filename suffixes etc. */

	if ((pos = path.find_last_of ('.')) != string::npos) {
		path = path.substr (0, pos);
	}

	ustring::size_type len = path.length();

	/* look for possible channel identifier: "?R", "%R", ".L" etc. */

	if (len > 3 && (path[len-2] == '%' || path[len-2] == '?' || path[len-2] == '.') &&
	    (path[len-1] == 'R' || path[len-1] == 'L' || (islower (path[len-1])))) {

		pair_base = path.substr (0, len-2);
		return true;

	}

	return false;
}

ustring
path_expand (ustring path)
{
#ifdef HAVE_WORDEXP
	/* Handle tilde and environment variable expansion in session path */
	string ret = path;

	wordexp_t expansion;
	switch (wordexp (path.c_str(), &expansion, WRDE_NOCMD|WRDE_UNDEF)) {
	case 0:
		break;
	default:
		error << string_compose (_("illegal or badly-formed string used for path (%1)"), path) << endmsg;
		goto out;
	}

	if (expansion.we_wordc > 1) {
		error << string_compose (_("path (%1) is ambiguous"), path) << endmsg;
		goto out;
	}

	ret = expansion.we_wordv[0];
  out:
	wordfree (&expansion);
	return ret;

#else
	return path;
#endif
}

#if defined(HAVE_COREAUDIO) || defined(HAVE_AUDIOUNITS)
string
CFStringRefToStdString(CFStringRef stringRef)
{
	CFIndex size =
		CFStringGetMaximumSizeForEncoding(CFStringGetLength(stringRef) ,
		kCFStringEncodingUTF8);
	    char *buf = new char[size];

	std::string result;

	if(CFStringGetCString(stringRef, buf, size, kCFStringEncodingUTF8)) {
	    result = buf;
	}
	delete [] buf;
	return result;
}
#endif // HAVE_COREAUDIO

void
compute_equal_power_fades (nframes_t nframes, float* in, float* out)
{
	double step;

	step = 1.0/nframes;

	in[0] = 0.0f;

	for (nframes_t i = 1; i < nframes - 1; ++i) {
		in[i] = in[i-1] + step;
	}

	in[nframes-1] = 1.0;

	const float pan_law_attenuation = -3.0f;
	const float scale = 2.0f - 4.0f * powf (10.0f,pan_law_attenuation/20.0f);

	for (nframes_t n = 0; n < nframes; ++n) {
		float inVal = in[n];
		float outVal = 1 - inVal;
		out[n] = outVal * (scale * outVal + 1.0f - scale);
		in[n] = inVal * (scale * inVal + 1.0f - scale);
	}
}

EditMode
string_to_edit_mode (string str)
{
	if (str == _("Splice Edit")) {
		return Splice;
	} else if (str == _("Slide Edit")) {
		return Slide;
	} else if (str == _("Lock Edit")) {
		return Lock;
	}
	fatal << string_compose (_("programming error: unknown edit mode string \"%1\""), str) << endmsg;
	/*NOTREACHED*/
	return Slide;
}

const char*
edit_mode_to_string (EditMode mode)
{
	switch (mode) {
	case Slide:
		return _("Slide Edit");

	case Lock:
		return _("Lock Edit");

	default:
	case Splice:
		return _("Splice Edit");
	}
}

SlaveSource
string_to_slave_source (string str)
{
	if (str == _("Internal")) {
		return None;
	}

	if (str == _("MTC")) {
		return MTC;
	}

	if (str == _("MIDI Clock")) {
		return MIDIClock;
	}

	if (str == _("JACK")) {
		return JACK;
	}

	fatal << string_compose (_("programming error: unknown slave source string \"%1\""), str) << endmsg;
	/*NOTREACHED*/
	return None;
}

const char*
slave_source_to_string (SlaveSource src)
{
	switch (src) {
	case JACK:
		return _("JACK");

	case MTC:
		return _("MTC");

	case MIDIClock:
		return _("MIDI Clock");

	default:
	case None:
		return _("Internal");

	}
}

/* I don't really like hard-coding these falloff rates here
 * Probably should use a map of some kind that could be configured
 * These rates are db/sec.
*/

#define METER_FALLOFF_OFF     0.0f
#define METER_FALLOFF_SLOWEST 6.6f // BBC standard
#define METER_FALLOFF_SLOW    8.6f // BBC standard
#define METER_FALLOFF_MEDIUM  20.0f
#define METER_FALLOFF_FAST    32.0f
#define METER_FALLOFF_FASTER  46.0f
#define METER_FALLOFF_FASTEST 70.0f

float
meter_falloff_to_float (MeterFalloff falloff)
{
	switch (falloff) {
	case MeterFalloffOff:
		return METER_FALLOFF_OFF;
	case MeterFalloffSlowest:
		return METER_FALLOFF_SLOWEST;
	case MeterFalloffSlow:
		return METER_FALLOFF_SLOW;
	case MeterFalloffMedium:
		return METER_FALLOFF_MEDIUM;
	case MeterFalloffFast:
		return METER_FALLOFF_FAST;
	case MeterFalloffFaster:
		return METER_FALLOFF_FASTER;
	case MeterFalloffFastest:
		return METER_FALLOFF_FASTEST;
	default:
		return METER_FALLOFF_FAST;
	}
}

MeterFalloff
meter_falloff_from_float (float val)
{
	if (val == METER_FALLOFF_OFF) {
		return MeterFalloffOff;
	}
	else if (val <= METER_FALLOFF_SLOWEST) {
		return MeterFalloffSlowest;
	}
	else if (val <= METER_FALLOFF_SLOW) {
		return MeterFalloffSlow;
	}
	else if (val <= METER_FALLOFF_MEDIUM) {
		return MeterFalloffMedium;
	}
	else if (val <= METER_FALLOFF_FAST) {
		return MeterFalloffFast;
	}
	else if (val <= METER_FALLOFF_FASTER) {
		return MeterFalloffFaster;
 	}
	else {
		return MeterFalloffFastest;
	}
}

float
meter_hold_to_float (MeterHold hold)
{
	switch (hold) {
	case MeterHoldOff:
		return 0.0f;
	case MeterHoldShort:
		return 40.0f;
	case MeterHoldMedium:
		return 100.0f;
	case MeterHoldLong:
	default:
		return 200.0f;
	}
}

AutoState
ARDOUR::string_to_auto_state (std::string str)
{
	if (str == X_("Off")) {
		return Off;
	} else if (str == X_("Play")) {
		return Play;
	} else if (str == X_("Write")) {
		return Write;
	} else if (str == X_("Touch")) {
		return Touch;
	}

	fatal << string_compose (_("programming error: %1 %2"), "illegal AutoState string: ", str) << endmsg;
	/*NOTREACHED*/
	return Touch;
}

string
ARDOUR::auto_state_to_string (AutoState as)
{
	/* to be used only for XML serialization, no i18n done */

	switch (as) {
	case Off:
		return X_("Off");
		break;
	case Play:
		return X_("Play");
		break;
	case Write:
		return X_("Write");
		break;
	case Touch:
		return X_("Touch");
	}

	fatal << string_compose (_("programming error: %1 %2"), "illegal AutoState type: ", as) << endmsg;
	/*NOTREACHED*/
	return "";
}

AutoStyle
ARDOUR::string_to_auto_style (std::string str)
{
	if (str == X_("Absolute")) {
		return Absolute;
	} else if (str == X_("Trim")) {
		return Trim;
	}

	fatal << string_compose (_("programming error: %1 %2"), "illegal AutoStyle string: ", str) << endmsg;
	/*NOTREACHED*/
	return Trim;
}

string
ARDOUR::auto_style_to_string (AutoStyle as)
{
	/* to be used only for XML serialization, no i18n done */

	switch (as) {
	case Absolute:
		return X_("Absolute");
		break;
	case Trim:
		return X_("Trim");
		break;
	}

	fatal << string_compose (_("programming error: %1 %2"), "illegal AutoStyle type: ", as) << endmsg;
	/*NOTREACHED*/
	return "";
}

extern "C" {
	void c_stacktrace() { stacktrace (cerr); }
}
