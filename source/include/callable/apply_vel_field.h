/*
 * This file is part of the SYMPLER package.
 * https://github.com/kauzlari/sympler
 *
 * Copyright 2002-2013, 
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


#ifndef __APPLY_VEL_FIELD_H
#define __APPLY_VEL_FIELD_H

#include "thermostat.h"
#include "function_fixed.h"

using namespace std;

class Phase;
class Simulation;

/*!
 * \a Callable adding a user-defined field to the velocities
 */
class ApplyVelField : public Thermostat
{
 protected:

  /*!
   * Adds the specified algebraic expression to the x-component of the velocity. See \a init() for the known symbols.
   */
  FunctionFixed m_velX;
  
  /*!
   * Adds the specified algebraic expression to the y-component of the velocity. See \a init() for the known symbols.
   */
  FunctionFixed m_velY;
  
  /*!
   * Adds the specified algebraic expression to the z-component of the velocity. See \a init() for the known symbols.
   */
  FunctionFixed m_velZ;


  /*!
   * The colour, this \a Thermostat should act on
   */
  size_t m_colour;
 
  /*!
   * The species, this \a Thermostat should act on
   */
  string m_species;
 
  /*!
   * Initialize the property list
   */
  void init();

 public:
  /*!
   * Constructor
   * @param sim Pointer to the main simulation object
   */
  ApplyVelField(Simulation* sim);

  /*!
   * Destructor
   */
  virtual ~ApplyVelField() {}

  /*!
   * Thermalize the system
   */
  virtual void thermalize(Phase* p);

  /*!
   * Setup this \a Thermostat
   */
  virtual void setup();
};

#endif

