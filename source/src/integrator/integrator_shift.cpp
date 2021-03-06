/*
 * This file is part of the SYMPLER package.
 * https://github.com/kauzlari/sympler
 *
 * Copyright 2002-2018, 
 * David Kauzlaric <david.kauzlaric@imtek.uni-freiburg.de>,
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


#include "gen_f.h"
#include "phase.h"
#include "threads.h"
#include "controller.h"
#include "simulation.h"
#include "integrator_shift.h"
#include "cell.h"

using namespace std;


#define M_CONTROLLER ((Controller*) m_parent)
#define M_SIMULATION ((Simulation*) M_CONTROLLER->parent())
#define M_PHASE M_SIMULATION->phase()

#define M_MANAGER M_PHASE->manager()
const Integrator_Register<IntegratorShift> integrator_shift("IntegratorShift");

//---- Constructors/Destructor ----

IntegratorShift::IntegratorShift(Controller *controller):IntegratorPosition(controller)
{
  m_disp.assign(0);
  init();
}


IntegratorShift::~IntegratorShift()
{
}


//---- Methods ----

void IntegratorShift::init()
{
  // some modules need to know whether there is an Integrator,
  // which changes positions, that's why the following
  m_properties.setClassName("IntegratorPosition");
  m_properties.setName("IntegratorShift");

  m_properties.setDescription
    ("Does not really perform any integration, but shifts the position "
     "r(t) of each particle of the given species according to the "
     "user-defined shift vector given by attribute 'shiftSymbol'. The "
     "displacement is updated accordingly, while the velocity v(t) "
     "stays untouched. the shift is performed in integration-step1, "
     "while IntegratorShift is inactive in integration-step2. Further "
     "information on the "
     "integration-steps, including their place in the total SYMPLER "
     "workflow, can be found with the help option \"--help workflow\"."
     );

  STRINGPC
    (displacement, m_displacement_name,
     "Full name of the displacement, usable as attribute in other modules");
  
  STRINGPC
    (symbol, m_displacement_symbol,
     "Symbol assigned to the displacement, usable in algebraic expressions");
  
  m_displacement_name = "displacement";
  m_displacement_symbol = "ds";
  
  STRINGPC
    (shiftSymbol, m_shift_symbol,
     "Symbol name of the externally computed particle shift used as "
     "input by this module. NOTE: If no other module stores a result "
     "into this variables, then it stays at (0, 0, 0), i.e., no shift."
     );
  
  m_shift_symbol = "undefined";
  
}


void IntegratorShift::setup()
{
  IntegratorPosition::setup();

  m_displacement_offset = 
    Particle::s_tag_format[m_colour].addAttribute
      (m_displacement_name,
       DataFormat::POINT,
       true,
       m_displacement_symbol).offset;

  if(m_shift_symbol == "undefined")
    throw gError("IntegratorShift::setup", "Attribute 'shiftSymbol' has value \"undefined\".");
  
  // The Integrator adds the symbol, because it is created before the other symbols;
  // the rest is the job of calculators and caches. If there are none, the shift 
  // should be always zero
  if(!Particle::s_tag_format[m_colour].attrExists(m_shift_symbol))
    m_shift_offset = 
      Particle::s_tag_format[m_colour].addAttribute
      (m_shift_symbol, DataFormat::POINT,
      false /*it's not an integrated quantity, so not persistent !!!*/)
      .offset;
  else
    throw gError("IntegratorShift::setup", "Cannot add symbol " + m_shift_symbol + " to species " + m_species + " because it is already used.");
  
}


void IntegratorShift::isAboutToStart()
{
  IntegratorPosition::isAboutToStart();
}


void IntegratorShift::integrateStep1()
{
  // position integration (will internally call integratePosition etc.)
  M_PHASE -> invalidatePositions(this);

  // Currently (2018-07-02) the Controller does not do this by itself
  // after integration step 2 (but after step 1), hence we do it here
  M_CONTROLLER -> triggerNeighbourUpdate();
}


void IntegratorShift::integrateStep2()
{ 
}


void IntegratorShift::integratePosition(Particle* p, Cell* cell)
{
  point_t& pDisp = p->tag.pointByOffset(m_displacement_offset);
  const point_t& pShift = p->tag.pointByOffset(m_shift_offset);

  // is not 0. but should be irrelevant for the collision algorithm
  // since the new shift is the only input for the new position
  const point_t accel = { 0., 0., 0. };
  // point_t accel = p->force[force_index] / p->m_mass;

  // will also compute m_disp and set a new (inverted and decreased)
  // shift in IntegratorShift::hitPos
  cell->doCollision(p, p->r, p->v, accel, (IntegratorPosition*) this);
  
  // the total shift or the remaining one (if there was a hit, then in
  // the opposite direction of the original one)  
  p->r += pShift;

  // m_disp = 0.0 if there was no hit
  pDisp += m_disp;

  // the net shift if there was a hit (and also if not)
  pDisp += pShift;
  
  // for next usage
  m_disp.assign(0);

  // inconsistency due to periodic BCs between the displacement and 
  // the position can not occur because the PBCs are only checked afterwards 
}


void IntegratorShift::integrateVelocity(Particle* p)
{
  // The velocity is kept constant and is not used for integration.
  // Maybe there is another IntegratorPosition doing this, maybe not... 
}


void IntegratorShift::solveHitTimeEquation(WallTriangle* wallTriangle, const Particle* p, const point_t &force, vector<double>* results)
{

  const point_t& shiftVec = p->tag.pointByOffset(m_shift_offset);
  const point_t& surface_normal = wallTriangle->normal();

  // The "*" between two point_t are scalar products!
  
  // shift component normal to plane of WallTriangle
  double normShift = surface_normal * shiftVec;

  // is there any shift-component normal to the wall?
  if(fabs(normShift) > 0.) {
    double ratio =
      // nominator = normal distance of p->r to plane of WallTriangle
      (surface_normal * p->r - wallTriangle->nDotR())
      / normShift;

    // add (subtract) a safety epsilon for following if-check
    ratio -= g_geom_eps;
    
    // "time" of the hit tHit = m_dt - remainingDtAfterHit
    // = m_dt - m_dt * (normalShift + normalDistToWall) / normalShift
    // = -m_dt * normalDistToWall / normalShift = -m_dt * ratio
    if (ratio < 0.) { // hence, tHit >= 0
      results->push_back(-m_dt * ratio);
      
      // NOTE: do NOT do the following line of code here, since the found
      // time might be too large such that the hit does not take place.
      // This is checked by the WallTriangle. Do this line in hitPos(..)
      // m_disp += shiftVec * ratio;
    } // else: see next else    
  }
  // else = no hit: the algorithm is fine with an empty list of hit
  // times, and we use the original shift
  // m_disp should still be 0 (meant to store the displacment up to the
  // hit)

}


void IntegratorShift::hitPos
(const double& dt, const Particle* p, point_t &hit_pos, const point_t &force)
{

  const point_t& shiftVec = p->tag.pointByOffset(m_shift_offset);
  // m_dt -- and also p->dt -- should hold the full time_step, since
  // the current collision algorithm implemented here does not permit
  // multiple collision events (not even near corners!). Hence we could
  // use p->dt but we do not have to.
  double timeToHitFrac = dt / m_dt;

  // m_disp must now have the value directly AT the hit position. The
  // remaining shift is taken care of later in integratePosition(..),
  // by first computing the remaining shift at the end of this function.
  // "+=" should be OK, since either m_disp was reset to 0.0 for this
  // particle, or we have to take multiple hits into account (which
  // should in fact not happen, since we just revert the shift direction.
  m_disp += timeToHitFrac * shiftVec;
  
  hit_pos = p->r + m_disp;

  // Remaining shift:
  // (Most) Reflectors do not touch the shift vector, so we decide to
  // revert it here, but at most such that the net shift of the
  // particle is zero.
  // So first we invert the direction: this is the "-".
  // Then we scale to at most the length that the particle was already
  // shifted by using the remaining dt. If the fist argument wins, then
  // the particle is just placed back to where it was.
  // The safety g_geom_eps was previously added to a dimensionless
  // ratio, hence we use it here in the same way
  p->tag.pointByOffset(m_shift_offset) *= -(min(dt, m_dt - dt) / m_dt + g_geom_eps);  
}


#ifdef _OPENMP
string IntegratorShift::dofIntegr() {
  return "vel_pos";
}


void IntegratorShift::mergeCopies(Particle* p, int thread_no, int force_index) {
  if (m_merge == true) {
    for (int i = 0; i < SPACE_DIMS; ++i) {
      p->force[force_index][i]
	+= (*p->tag.vectorDoubleByOffset
	    (m_vec_offset[thread_no])
	    )[m_vec_pos + i];

      (*p->tag.vectorDoubleByOffset(m_vec_offset[thread_no]))
	[m_vec_pos + i] = 0.;
    }
  }
}

#endif

