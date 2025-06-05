
/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------

Description: Utilities for debugging input synchronization problems

-------------------------------------------------------------------------
History:
-	30:03:2007  : Created by Craig Tiller

*************************************************************************/

#ifndef __NETINPUTCHAINDEBUG_H__
#define __NETINPUTCHAINDEBUG_H__

#if !defined(LINUX)
	#define ENABLE_NETINPUTCHAINDEBUG 1
#endif

void NetInputChainInitCVars();

#if ENABLE_NETINPUTCHAINDEBUG
void NetInputChainPrint( const char * name, float val );
void NetInputChainPrint( const char * name, Vec3 val );

//TheOtherSide
void NetInputChainPrint( const char * name, string val );
//~TheOtherSide

extern EntityId _netinputchain_debugentity;

#define NETINPUT_TRACE(ent, val) if (ent != _netinputchain_debugentity); else NetInputChainPrint(#val, val)
#else
#define NETINPUT_TRACE(ent, val) ((void*)0)
#endif

#endif
