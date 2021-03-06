/*
 * This file is part of the SYMPLER package.
 * https://github.com/kauzlari/sympler
 *
 * Copyright 2002-2018, 
 * David Kauzlaric <david.kauzlaric@frias.uni-freiburg.de>,
 * and others authors stated in the AUTHORS file in the top-level 
 * source directory.
 *
 * SYMPLER is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SYMPLER is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SYMPLER.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Please cite the research papers on SYMPLER in your own publications. 
 * Check out the PUBLICATIONS file in the top-level source directory.
 *
 * You are very welcome to contribute extensions to the code. Please do 
 * so by making a pull request on https://github.com/kauzlari/sympler
 * 
 */


#ifndef __SHIFT_PARTICLE_H
#define __SHIFT_PARTICLE_H

#include "thermostat.h"

using namespace std;

class Phase;
class Simulation;

/*!
 * \a Callable shifting particles by a user-defined particle-specific 
 * displacement vector
 */
class ShiftParticle : public Thermostat
{

 protected:

  /*!
   * The colour, this \a Callable should act on
   */
  size_t m_colour;
 
  /*!
   * The species, this \a Callable should act on
   */
  string m_species;

  /*!
   * Name of the vector symbol holding the shift for each particle
   */
  string m_shiftSymbolName;

  /*!
   * Memmory offset in \a Particle.tag where the particle shift vector
   * is stored
   */
  size_t m_shiftOffset;
  
  /*!
   * Initialize the property list
   */
  void init();

  
 public:
  /*!
   * Constructor
   * @param sim Pointer to the main simulation object
   */
  ShiftParticle(Simulation* sim);

  /*!
   * Destructor
   */
  virtual ~ShiftParticle() {}

  /*!
   * The actual shifting operation
   * FIXME: Generalise deprecated name
   */
  virtual void thermalize(Phase* p);

  /*!
   * Setup this module
   */
  virtual void setup();
};

#endif

