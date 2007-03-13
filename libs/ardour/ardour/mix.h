/*
    Copyright (C) 2005 Sampo Savolainen

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
#ifndef __ardour_mix_h__
#define __ardour_mix_h__

#include <ardour/types.h>
#include <ardour/utils.h>
#include <ardour/io.h>

#if defined (ARCH_X86) && defined (BUILD_SSE_OPTIMIZATIONS)

extern "C" {
/* SSE functions */
	float x86_sse_compute_peak		(ARDOUR::Sample *buf, nframes_t nsamples, float current);

	void  x86_sse_apply_gain_to_buffer	(ARDOUR::Sample *buf, nframes_t nframes, float gain);

	void  x86_sse_mix_buffers_with_gain	(ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes, float gain);

	void  x86_sse_mix_buffers_no_gain	(ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes);
}

float x86_sse_find_peaks                        (ARDOUR::Sample *buf, nframes_t nsamples, float *min, float *max);

/* debug wrappers for SSE functions */

float debug_compute_peak		(ARDOUR::Sample *buf, nframes_t nsamples, float current);

void  debug_apply_gain_to_buffer	(ARDOUR::Sample *buf, nframes_t nframes, float gain);

void  debug_mix_buffers_with_gain	(ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes, float gain);

void  debug_mix_buffers_no_gain		(ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes);

#endif

#if defined (__APPLE__)

float veclib_compute_peak              (ARDOUR::Sample *buf, nframes_t nsamples, float current);

float veclib_find_peaks                (ARDOUR::Sample *buf, nframes_t nsamples, float *min, float *max);

void  veclib_apply_gain_to_buffer      (ARDOUR::Sample *buf, nframes_t nframes, float gain);

void  veclib_mix_buffers_with_gain     (ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes, float gain);

void  veclib_mix_buffers_no_gain       (ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes);

#endif

/* non-optimized functions */

float compute_peak              (ARDOUR::Sample *buf, nframes_t nsamples, float current);

float find_peaks                (ARDOUR::Sample *buf, nframes_t nsamples, float *min, float *max);

void  apply_gain_to_buffer      (ARDOUR::Sample *buf, nframes_t nframes, float gain);

void  mix_buffers_with_gain     (ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes, float gain);

void  mix_buffers_no_gain       (ARDOUR::Sample *dst, ARDOUR::Sample *src, nframes_t nframes);

#endif /* __ardour_mix_h__ */
