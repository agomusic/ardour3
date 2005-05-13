// $Id$
//
// Cassowary Incremental Constraint Solver
// Original Smalltalk Implementation by Alan Borning
// This C++ Implementation by Greg J. Badros, <gjb@cs.washington.edu>
// http://www.cs.washington.edu/homes/gjb
// (C) 1998, 1999 Greg J. Badros and Alan Borning
// See ../LICENSE for legal details regarding this software
//
// ClFDVariable.cc

#include <cassowary/ClFDVariable.h>
#include <cassowary/ClSolver.h> // for list<FDNumber> printing

#ifdef HAVE_CONFIG_H
#include <config.h>
#define CONFIG_H_INCLUDED
#endif

// Use < > for ClFDVariable-s, instead of [ ]
#ifndef CL_NO_IO
ostream &ClFDVariable::PrintOn(ostream &xo) const
{  
  xo << "<" << Name() << "=" << Value() << ":" << *PlfdnDomain() << ">";
  return xo;
}
#endif
