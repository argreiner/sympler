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


#include "gen_f.h"
#include "phase.h"
#include "threads.h"
#include "particle.h"
#include "controller.h"
#include "simulation.h"
#include "integrator_tensor.h"

using namespace std;


#define M_CONTROLLER  ((Controller*) m_parent)
#define M_SIMULATION  ((Simulation*) M_CONTROLLER->parent())
#define M_PHASE  M_SIMULATION->phase()


const Integrator_Register<IntegratorTensor> integrator_tensor("IntegratorTensor");

//---- Constructors/Destructor ----

IntegratorTensor::IntegratorTensor(Controller *controller): Integrator(controller)
{
  init();
}


IntegratorTensor::~IntegratorTensor()
{
}



//---- Methods ----

void IntegratorTensor::init()
{
//   MSG_DEBUG("IntegratorTensor::init()", "running");

  m_properties.setClassName("IntegratorTensor");

  m_properties.setDescription(
      "Adds an additional tensorial degree of freedom, to the particles specified. Integration "
      "is performed with a simple Euler scheme."
                             );

  STRINGPC
      (tensor, m_tensor_name,
       "Full name of the additional tensor field, usable as attribute in other modules");

  STRINGPC
      (symbol, m_tensor_symbol,
       "Symbol assigned to the additional tensor field, usable in algebraic expressions");

  m_tensor_name = "tensor";
  m_tensor_symbol = "sc";
}


void IntegratorTensor::setup()
{
  Integrator::setup();

  DataFormat::attribute_t tmpAttr;

  m_tensor_offset =
      Particle::s_tag_format[m_colour].addAttribute
      (m_tensor_name,
       DataFormat::TENSOR,
       true,
       m_tensor_symbol).offset;

  for(size_t i = 0; i < FORCE_HIST_SIZE; ++i)
  {
    tmpAttr =
        Particle::s_tag_format[m_colour].addAttribute
        (STR_FORCE + STR_DELIMITER + m_tensor_name + STR_DELIMITER + ObjToString(i),
         DataFormat::TENSOR,
         true);

    m_force_offset[i] = tmpAttr.offset;
    m_fAttr_index[i] = tmpAttr.index;
  }
  m_dt = M_CONTROLLER->dt();

  /*  MSG_DEBUG("IntegratorTensor::setup", "m_dt = " << m_dt << ", tensor=" << m_tensor_name << "\n in Particle: " << Particle::s_tag_format[m_colour].toString() << "\nm_colour=" << m_colour);*/
}

void IntegratorTensor::isAboutToStart()
{
  Phase *phase = M_PHASE;

  FOR_EACH_FREE_PARTICLE_C__PARALLEL
      (phase, m_colour, this,
       for (int j = 0; j < FORCE_HIST_SIZE; ++j)
           i->tag.tensorByOffset(((IntegratorTensor*) data)->m_force_offset[j]).assign(0);
      );

}

void IntegratorTensor::unprotect(size_t index)
{
  Phase *phase = M_PHASE;

  FOR_EACH_FREE_PARTICLE_C__PARALLEL
      (phase, m_colour, this,
       i->tag.unprotect(m_fAttr_index[index]);
       if(!index) i->tag.protect(m_fAttr_index[FORCE_HIST_SIZE-1]);
       else i->tag.protect(m_fAttr_index[index-1]);

      );

}



void IntegratorTensor::integrateStep1()
{
  Phase *phase = M_PHASE;

  //  MSG_DEBUG("IntegratorTensor::integrateStep1", "m_dt = " << m_dt);
//   m_force_index = M_CONTROLLER->forceIndex();
  size_t force_index = M_CONTROLLER->forceIndex();

  FOR_EACH_FREE_PARTICLE_C__PARALLEL
      (phase, m_colour, this,

       for(size_t j = 0; j < SPACE_DIMS; ++j)
         for(size_t k = 0; k < SPACE_DIMS; ++k)
       {
         if (std::isnan(i->tag.tensorByOffset(((IntegratorTensor*) data)->m_force_offset[/*m_*/force_index])(j, k))) {
           cout << "slot = " << i->mySlot << ", "
               << ((IntegratorTensor*) data)->m_tensor_name << " = "
               << i->tag.tensorByOffset(((IntegratorTensor*) data)->m_tensor_offset) << ", "
               << "force = "
               << i->tag.tensorByOffset(((IntegratorTensor*) data)->m_force_offset[/*m_*/force_index])
               << endl;

           throw gError("IntegratorTensor::integrateStep1", "Force was not-a-number!");
         }

         i->tag.tensorByOffset(((IntegratorTensor*) data)->m_tensor_offset)(j, k) +=
             ((IntegratorTensor*) data)->m_dt * i->tag.tensorByOffset(((IntegratorTensor*) data)->m_force_offset[/*m_*/force_index])(j, k);
       }
      );

}


void IntegratorTensor::integrateStep2()
{
}


#ifdef _OPENMP
string IntegratorTensor::dofIntegr() {
  return m_tensor_name;
}


void IntegratorTensor::mergeCopies(Particle* p, int thread_no, int force_index) {
  if (m_merge == true) {
    size_t _i = 0;
    for(size_t a = 0; a < SPACE_DIMS; ++a) {
      for(size_t b = 0; b < SPACE_DIMS; ++b) {
        p->tag.tensorByOffset(this->m_force_offset[force_index])(a, b) += (*p->tag.vectorDoubleByOffset(m_vec_offset[thread_no]))[m_vec_pos + _i];
        (*p->tag.vectorDoubleByOffset(m_vec_offset[thread_no]))[m_vec_pos + _i] = 0;
        ++_i;
      }
    }
  }
}
#endif

