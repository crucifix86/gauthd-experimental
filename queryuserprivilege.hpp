
#ifndef __GNET_QUERYUSERPRIVILEGE_HPP
#define __GNET_QUERYUSERPRIVILEGE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"
#include "gmysqlclient.hpp"
#include "queryuserprivilege_re.hpp"
namespace GNET
{

class QueryUserPrivilege : public GNET::Protocol
{
	#include "queryuserprivilege"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		// TODO
		QueryUserPrivilege_Re qup_re;
		qup_re.userid=userid;
		MysqlManager::GetInstance()->QueryGMPrivilege(userid, zoneid, qup_re.auth);
		manager->Send(sid,qup_re);
	}
};

};

#endif
