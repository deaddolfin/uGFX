/*
 * Copyright (c) 2012, 2013, Joel Bodenmann aka Tectu <joel@unormal.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * --------------------------- Our Custom GWIN Oscilloscope ---------------
 *
 * This GWIN superset implements a simple audio oscilloscope using the GAUDIN module.
 *
 * It makes many assumptions, the most fundamental of which is that the audio device
 * produces unsigned integer samples.
 *
 * The GMISC module with GMISC_NEED_ARRAYOPS could be used to process the samples more
 * correctly if we were really building something generic.
 */

#include "gfx.h"
#include "gwinosc.h"

/* Include internal GWIN routines so we can build our own superset class */
#include "gwin/internal.h"

/* Our GWIN identifier */
#define GW_SCOPE				(GW_FIRST_USER_WINDOW+0)

/* The size of our dynamically allocated audio buffer */
#define AUDIOBUFSZ				64*2

/* How many flat-line sample before we trigger */
#define FLATLINE_SAMPLES		8

GHandle gwinCreateScope(GScopeObject *gs, coord_t x, coord_t y, coord_t cx, coord_t cy, uint16_t channel, uint32_t frequency) {
	/* Initialise the base class GWIN */
	if (!(gs = (GScopeObject *)_gwindowCreate((GWindowObject *)gs, x, y, cx, cy, sizeof(GScopeObject))))
		return 0;

	/* Initialise the scope object members and allocate memory for buffers */
	gs->gwin.type = GW_SCOPE;
	gfxSemInit(&gs->bsem, 0, 1);
	gs->nextx = 0;
	if (!(gs->lastscopetrace = (coord_t *)gfxAlloc(NULL, gs->gwin.width * sizeof(coord_t))))
		return 0;
	if (!(gs->audiobuf = (adcsample_t *)gfxAlloc(NULL, AUDIOBUFSZ * sizeof(adcsample_t))))
		return 0;
#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP
	gs->lasty = gs->gwin.height/2;
#elif TRIGGER_METHOD == TRIGGER_MINVALUE
	gs->lasty = gs->gwin.height/2;
	gs->scopemin = 0;
#endif

	/* Start the GADC high speed converter */
	gaudinInit(channel, frequency, gs->audiobuf, AUDIOBUFSZ, AUDIOBUFSZ/2);
	gaudinSetBSem(&gs->bsem, &gs->myEvent);
	gaudinStart();

	return (GHandle)gs;
}

void gwinWaitForScopeTrace(GHandle gh) {
	#define 		gs	((GScopeObject *)(gh))
	int				i;
	coord_t			x, y;
	coord_t			yoffset;
	audin_sample_t	*pa;
	coord_t			*pc;
#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP
	bool_t			rdytrigger;
	int				flsamples;
#elif TRIGGER_METHOD == TRIGGER_MINVALUE
	bool_t			rdytrigger;
	int				flsamples;
	coord_t			scopemin;
#endif

	/* Wait for a set of audio conversions */
	gfxSemWait(&gs->bsem, TIME_INFINITE);

	/* Ensure we are drawing in the right area */
	#if GDISP_NEED_CLIP
		gdispSetClip(gh->x, gh->y, gh->width, gh->height);
	#endif

	yoffset = gh->height/2 + (1<<SCOPE_Y_BITS)/2;
	x = gs->nextx;
	pc = gs->lastscopetrace+x;
	pa = gs->myEvent.buffer;
#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP
	rdytrigger = FALSE;
	flsamples = 0;
#elif TRIGGER_METHOD == TRIGGER_MINVALUE
	rdytrigger = FALSE;
	flsamples = 0;
	scopemin = 0;
#endif

	for(i = gs->myEvent.count; i; i--) {

		/* Calculate the new scope value - re-scale using simple shifts for efficiency, re-center and y-invert */
		#if GAUDIN_BITS_PER_SAMPLE > SCOPE_Y_BITS
			y = yoffset - (*pa++ >> (GAUDIN_BITS_PER_SAMPLE - SCOPE_Y_BITS));
		#else
			y = yoffset - (*pa++ << (SCOPE_Y_BITS - GAUDIN_BITS_PER_SAMPLE));
		#endif

#if TRIGGER_METHOD == TRIGGER_MINVALUE
		/* Calculate the scopemin ready for the next trace */
		if (y > scopemin)
			scopemin = y;
#endif

		/* Have we reached the end of a scope trace? */
		if (x >= gh->width) {

#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP || TRIGGER_METHOD == TRIGGER_MINVALUE
			/* Handle triggering - we trigger on the next sample minimum (y value maximum) or a flat-line */

			#if TRIGGER_METHOD == TRIGGER_MINVALUE
				/* Arm when we reach the sample minimum (y value maximum) of the previous trace */
				if (!rdytrigger && y >= gs->scopemin)
					rdytrigger = TRUE;
			#endif

			if (y == gs->lasty) {
				/* Trigger if we get too many flat-line samples regardless of the armed state */
				if (++flsamples < FLATLINE_SAMPLES)
					continue;
				flsamples = 0;
			} else if (y > gs->lasty) {
				gs->lasty = y;
				flsamples = 0;
				#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP
					/* Arm the trigger when samples fall (y increases) ie. negative slope */
					rdytrigger = TRUE;
				#endif
				continue;
			} else {
				/* If the trigger is armed, Trigger when samples increases (y decreases) ie. positive slope */
				gs->lasty = y;
				flsamples = 0;
				if (!rdytrigger)
					continue;
			}

			/* Ready for a the next trigger cycle */
			rdytrigger = FALSE;
#endif

			/* Prepare for a scope trace */
			x = 0;
			pc = gs->lastscopetrace;
		}

		/* Clear the old scope pixel and then draw the new scope value */
		gdispDrawPixel(gh->x+x, gh->y+pc[0], gh->bgcolor);
		gdispDrawPixel(gh->x+x, gh->y+y, gh->color);

		/* Save the value */
		*pc++ = y;
		x++;
		#if TRIGGER_METHOD == TRIGGER_POSITIVERAMP || TRIGGER_METHOD == TRIGGER_MINVALUE
			gs->lasty = y;
		#endif
	}
	gs->nextx = x;
#if TRIGGER_METHOD == TRIGGER_MINVALUE
	gs->scopemin = scopemin;
#endif

	#undef gs
}
