#ifndef HLIDAC_PES_CONSTANTS_RCPSP_H
#define HLIDAC_PES_CONSTANTS_RCPSP_H

/*!
 * \file ConstantsRCPSP.h
 * \author Libor Bukata
 * \brief Declare important constants for RCPSP.
 */

/* DEFINE BASIC RCPSP CONSTANTS - DON'T EDIT! */

//! Possible moves in a space.
enum MoveType {
	SWAP, SHIFT, NONE
};

//! Types of the tabu list.
enum TabuType {
	SIMPLE_TABU, ADVANCED_TABU 
};

#endif

