
#ifndef __GNET_PANELRESPONSE_RE_HPP
#define __GNET_PANELRESPONSE_RE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"


namespace GNET
{

class PanelResponse_Re : public GNET::Protocol
{
	#include "panelresponse_re"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		// TODO
	}
};

};

#endif
