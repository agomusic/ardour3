// $Id$
//
// Cassowary Incremental Constraint Solver
// Original Smalltalk Implementation by Alan Borning
// This C++ Implementation by Greg J. Badros, <gjb@cs.washington.edu>
// http://www.cs.washington.edu/homes/gjb
// (C) 1998, 1999 Greg J. Badros and Alan Borning
// See ../LICENSE for legal details regarding this software
//
// ClVariable.cc

#include <cassowary/ClVariable.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#define CONFIG_H_INCLUDED
#endif

StringToVarMap *ClVariable::pmapStrPclv = NULL;
ClVariable clvNil(static_cast<ClAbstractVariable*>(0));

