
#ifndef __GNET_PANELCHALLENGE_HPP
#define __GNET_PANELCHALLENGE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"


namespace GNET
{

class PanelChallenge : public GNET::Protocol
{
	#include "panelchallenge"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		// TODO
	}
};

};

#endif
