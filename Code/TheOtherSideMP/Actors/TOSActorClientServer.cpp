#include "StdAfx.h"
#include "TOSActor.h"
#include <TheOtherSideMP/Helpers/TOS_Entity.h>
#include <TheOtherSideMP/Helpers/TOS_Script.h>
#include <Fists.h>
//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClTOSJump)
{
	CMovementRequest request;
	request.SetJump();
	GetMovementController()->RequestMovement(request);

	CryLog("[%s] Received jump", m_debugName);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, SvRequestTOSJump)
{
	auto channelId = g_pGame->GetIGameFramework()->GetGameChannelId(pNetChannel);
	GetGameObject()->InvokeRMI(ClTOSJump(), params, eRMI_ToOtherClients | eRMI_NoLocalCalls, channelId);
	GetGameObject()->Pulse('bang');

	if ((IsSlave() && !IsLocalSlave()) || IsClient())
	{
		CMovementRequest request;
		request.SetJump();
		GetMovementController()->RequestMovement(request);

		CryLog("[%s] Requesting jump to channel %i", m_debugName, channelId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClAttachChild)
{
	IEntity* pChild = TOS_GET_ENTITY(params.id);
	if (pChild)
	{
		GetEntity()->AttachChild(pChild, params.flags);
		CryLog("[%s] Received attach child '%s'", m_debugName, pChild->GetName());
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, SvRequestAttachChild)
{
	IEntity* pChild = TOS_GET_ENTITY(params.id);
	if (pChild)
	{
		CryLog("[%s] Requesting attach child '%s'", m_debugName, pChild->GetName());
		GetEntity()->AttachChild(pChild, params.flags);
	}

	GetGameObject()->InvokeRMI(ClAttachChild(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CTOSActor, ClClearInventory)
{
	CryLog("[%s] Received inventory clear", m_debugName);

	RemoveAllItems();
	return true;
}
