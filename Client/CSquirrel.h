#ifndef _CSQUIRREL_H
#define _CSQUIRREL_H
/**
	Lost Heaven Multiplayer

	Purpose: client-side implementation of scripts

	@author Romop5
	@version 1.0 27/04/15
*/

#include <stdarg.h>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <iostream>
#include <vector>

#include <sqstdstring.h>
#include <sqstdmath.h>
#include <sqstdsystem.h>

/*#ifdef SQUNICODE
#define scvprintf vwprintf
#else
#define scvprintf vprintf
#endif
void printfunc(HSQUIRRELVM v, const SQChar *s, ...);


#include "sq_functions.h"
*/

// defined below
class CScript;
class CSquirrel
{
public:
	HSQUIRRELVM p_VM;
	// Constructor
	CSquirrel();
	// Startup a new client script
	// IF SUCCESS, new script is added into script pool
	void LoadClientScript(char* scriptname);

	// Delete all scripts
	void DeleteScripts();

	/* --------------- Callbacks ------------*/
	// called when game renders
	void onRender();
private:
	// Register all native functions and constants
	void PrepareMachine(SQVM* pVM);
	// functions used to register constants and functions, used in InitVM
	//void RegisterFunction(HSQUIRRELVM pVM, char * szFunc, SQFUNCTION func, int params, const char * szTemplate);
	void RegisterVariable(HSQUIRRELVM pVM, const char * szVarName, bool bValue);
	void RegisterVariable(HSQUIRRELVM pVM, const char * szVarName, int iValue);
	void RegisterVariable(HSQUIRRELVM pVM, const char * szVarName, float fValue);
	void RegisterVariable(HSQUIRRELVM pVM, const char * szVarName, const char * szValue);

	// client-side scripts pool
	//std::vector <CScript> p_scriptPool;
	CScript* p_scriptPool[100];

};

class CScript
{
public:
	// Create new instance with @scriptname name and @pvm virtual machine instance
	CScript(HSQUIRRELVM pvm)
	{
		this->p_VM = pvm;
	}
	~CScript()
	{
		if (this->p_VM)
			sq_close(this->p_VM);
	}
	HSQUIRRELVM GetVirtualMachine()
	{
		return this->p_VM;
	}
private:
	HSQUIRRELVM p_VM;
};

#endif