/*
 *  Adjunct
 *  
 *  Copyright 2015 Kustaa Kangas <jwkangas(at)cs.helsinki.fi>
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <cmath>

#include "tools.hpp"

// returns ln(e^x + e^y) in a numerically stable way,
// i.e., works even when e^x or e^y would be too large
double logsum(double x, double y)
{
	if (x > y) {
		return x + log(1.0 + exp(y - x));
	} else {
		return y + log(1.0 + exp(x - y));
	}
}

double rnd()
{
	return rand() / ((double)RAND_MAX + 1);
}
