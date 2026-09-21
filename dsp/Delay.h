/*
	dsp/Delay.h
	
	Copyright 2003-17 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	delay lines with fractional (linear or cubic interpolation) lookup 

	delay line storage is aligned to powers of two for simplified wrapping
	checks (no conditional or modulo, binary 'and' suffices instead).

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

#ifndef DSP_DELAY_H
#define DSP_DELAY_H

#include "util.h"
#include "FPTruncateMode.h"

namespace DSP {
	
class Delay
{
	public:
		uint size;
		sample_t * data;
		uint read, write;

		Delay() { read = write = 0; data = 0; }

		~Delay() { free (data); }

		void init(uint n)
			{
			#ifdef PICOLV2
				/* Exact-sized circular storage saves about 60 KiB in Plate at
				 * 48 kHz.  The desktop implementation retains mask-based power-
				 * of-two wrapping. */
				size = n;
				assert(size > 0 && size <= (1 << 20));
				data = (sample_t *) calloc(sizeof (sample_t), size);
				read = write = 0;
			#else
				size = next_power_of_2(n);
				assert(size <= (1 << 20));
				data = (sample_t *) calloc(sizeof (sample_t), size);
				--size; /* used as mask for confining access */
				write = n;
			#endif
			}

		void reset()
			{
			#ifdef PICOLV2
				memset(data, 0, size * sizeof (sample_t));
			#else
				memset(data, 0, (size + 1) * sizeof (sample_t));
			#endif
			}

	#ifdef PICOLV2
		sample_t & operator [] (int i)
			{
				int index = (int) write - i;
				while (index < 0) index += size;
				return data[index];
			}
	#else
		sample_t & operator [] (int i) { return data [(write - i) & size]; }
	#endif

		inline void put(sample_t x)
			{
				data [write] = x;
			#ifdef PICOLV2
				if (++write == size) write = 0;
			#else
				write = (write + 1) & size;
			#endif
			}

		inline sample_t get()
			{
				sample_t x = data [read];
			#ifdef PICOLV2
				if (++read == size) read = 0;
			#else
				read = (read + 1) & size;
			#endif
				return x;
			}
		inline sample_t peek() { return data [read]; }
	#ifdef PICOLV2
		inline sample_t putget (sample_t x)
			{
				sample_t y = get();
				put(x);
				return y;
			}
	#else
		inline sample_t putget (sample_t x) {put(x); return get();}
	#endif

		/* fractional lookup, linear interpolation */
		inline sample_t get_linear (float f)
			{
				int n;
				fistp (f, n); /* read: i = (int) f; relies on FPTruncateMode */
				f -= n;
				
				return (1 - f) * (*this) [n] + f * (*this) [n + 1];
			}

		/* fractional lookup, cubic interpolation */
		inline sample_t get_cubic (float f)
			{
				int n;
				fistp (f, n); /* see FPTruncateMode */
				f -= n;

				sample_t x_1 = (*this) [n - 1];
				sample_t x0 = (*this) [n];
				sample_t x1 = (*this) [n + 1];
				sample_t x2 = (*this) [n + 2];

				/* sample_t (32bit) quicker than double here */
				register sample_t a = 
						(3 * (x0 - x1) - x_1 + x2) * .5;
				register sample_t b =
						2 * x1 + x_1 - (5 * x0 + x2) * .5;
				register sample_t c = 
						(x1 - x_1) * .5;

				return x0 + (((a * f) + b) * f + c) * f;
			}
};

class MovingAverage
: public Delay
{
	public:
		sample_t state, over_n;

		void init (uint n)
			{
				this->Delay::init (n);
				over_n = 1. / n;
				/* adjust write pointer so we have a full history of zeros */
			#ifndef PICOLV2
				write = (write + size + 1) & size;
			#endif
				state = 0;
			}

		void reset()
			{
				this->Delay::reset();
				state = 0;
			}

		void process (sample_t x) 
			{ 
				x *= over_n;
				state -= this->Delay::get();
				state += x;
				this->Delay::put (x); 
			}

		sample_t get() { return state; }
};

}; /* namespace DSP */

#endif /* DSP_DELAY_H */
