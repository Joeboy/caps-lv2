/*
	Reverb.h
	
	Copyright 2002-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	two reverb units: JVRev and Plate.
	
	the former is a rewrite of STK's JVRev, a traditional design.
	
	original comment:
	
		This is based on some of the famous    
		Stanford CCRMA reverbs (NRev, KipRev)  
		all based on the Chowning/Moorer/      
		Schroeder reverberators, which use     
		networks of simple allpass and comb    
		delay filters.  

	(STK is an effort of Gary Scavone).
	
	the algorithm is mostly unchanged in this implementation; the delay
	line lengths have been fiddled with to make the stereo field more
	evenly weighted, and denormal protection has been added.

	the Plate reverb is based on the circuit discussed in Jon Dattorro's 
	september 1997 JAES paper on effect design (part 1: reverb & filters).
*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef REVERB_H
#define REVERB_H

#include <stdio.h>

#include "dsp/Delay.h"
#include "dsp/IIR1.h"
#include "dsp/Sine.h"
#include "dsp/util.h"

#ifdef PICOLV2
typedef sample_t reverb_real_t;

/* The Cortex-M33 has a single-precision FPU, but no double-precision FPU.
 * Keep the Plate modulation oscillator in hardware float on PicoLV2; the
 * desktop build retains DSP::Sine's original double-precision recurrence. */
class ReverbSine
{
	public:
		sample_t sine, cosine;
		sample_t sine_step, cosine_step;
		uint normalize_counter;

		ReverbSine()
		: sine(0), cosine(1), sine_step(0), cosine_step(1),
		  normalize_counter(0)
			{}

		void set_f (double f, double fs, double phase)
		{
			double w = f*2*M_PI/fs;
			sine_step = (sample_t) sin(w);
			cosine_step = (sample_t) cos(w);
			sine = (sample_t) sin(phase - w);
			cosine = (sample_t) cos(phase - w);
			normalize_counter = 0;
		}

		inline sample_t get()
		{
			const sample_t old_sine = sine;
			sine = old_sine*cosine_step + cosine*sine_step;
			cosine = cosine*cosine_step - old_sine*sine_step;

			/* cosine_step rounds to one at very low frequencies.  Correct the
			 * resulting slow amplitude drift outside the usual audio block. */
			if ((++normalize_counter & 4095) == 0)
			{
				const sample_t scale = 1/sqrtf(sine*sine + cosine*cosine);
				sine *= scale;
				cosine *= scale;
			}
			return sine;
		}
};
#else
typedef double reverb_real_t;
typedef DSP::Sine ReverbSine;
#endif

/* both reverbs use this */
class Lattice
: public DSP::Delay
{
	public:
		sample_t process (sample_t x, reverb_real_t d)
			{
				sample_t y = get();
				x -= d*y;
				put(x);
				return d*x + y;
			}
};

/* helper for JVRev */
class JVComb
: public DSP::Delay
{
	public:
		float c;
		
		sample_t process (sample_t x)
			{
				x += c*get();
				put(x);
				return x;
			}
};

class JVRev
: public Plugin
{
	public:
		DSP::LP1<sample_t> bandwidth, tone;

		sample_t t60;

		int length[9];

		Lattice allpass[3];
		JVComb comb[4];

		DSP::Delay left, right;
		
		reverb_real_t apc;
		
		void cycle (uint frames);

		void set_t60 (sample_t t);

	public:
		static PortInfo port_info [];

		void init();
		void activate();
};

/* /////////////////////////////////////////////////////////////////////// */

class ModLattice
{
	public:
		float n0, width;

		DSP::Delay delay;
		ReverbSine lfo;
		
		void init (int n, int w)
			{
				n0 = n;
				width = w;
				delay.init (n + w);
			}

		void reset()
			{
				delay.reset();
			}

		inline sample_t
		process (sample_t x, reverb_real_t d)
			{
				sample_t y = delay.get_linear (n0 + width * lfo.get());
				x += d * y;
				delay.put (x);
				return y - d * x; /* note sign */
			}
};

class PlateStub
: public Plugin
{
	public:
		sample_t f_lfo;

		sample_t indiff1, indiff2, dediff1, dediff2;
		
		struct {
			DSP::LP1<sample_t> bandwidth;
			Lattice lattice[4];
		} input;

		struct {
			ModLattice mlattice[2];
			Lattice lattice[2];
			DSP::Delay delay[4];
			DSP::LP1<sample_t> damping[2];
			int taps[12];
		} tank;

	public:
		void init();
		void activate()
			{ 
				input.bandwidth.reset();

				for (int i = 0; i < 4; ++i)
				{
					input.lattice[i].reset();
					tank.delay[i].reset();
				}

				for (int i = 0; i < 2; ++i)
				{
					tank.mlattice[i].reset();
					tank.lattice[i].reset();
					tank.damping[i].reset();
				}
				
				tank.mlattice[0].lfo.set_f (1.2, fs, 0);
				tank.mlattice[1].lfo.set_f (1.2, fs, .5 * M_PI);
			}

		inline void process (sample_t x, sample_t decay, 
					sample_t * xl, sample_t * xr);
};

/* /////////////////////////////////////////////////////////////////////// */

class Plate
: public PlateStub
{
	public:
		void cycle (uint frames);

	public:
		static PortInfo port_info [];
};

/* /////////////////////////////////////////////////////////////////////// */

class PlateX2
: public PlateStub
{
	public:
		void cycle (uint frames);

	public:
		static PortInfo port_info [];
};

#endif /* REVERB_H */
