
#ifndef __GNET_ANNOUNCEZONEID3_HPP
#define __GNET_ANNOUNCEZONEID3_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"
#include "gmysqlclient.hpp"
#include "gauthserver.hpp"
namespace GNET
{

class AnnounceZoneid3 : public GNET::Protocol
{
	#include "announcezoneid3"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		// TODO
		Thread::Mutex::Scoped l(GAuthServer::GetInstance()->locker_zonemap);
		GAuthServer::GetInstance()->zonemap[sid]=zoneid;
		MysqlManager::GetInstance()->ClearOnlineRecord(sid,zoneid,aid);
		printf("gauthd::annoucezoneid3: zoneid=%d,aid=%d belongs to session %d\n",zoneid,aid,sid);
	}
};

};

#endif
