
#ifndef __GNET_ADDCASH_RE_HPP
#define __GNET_ADDCASH_RE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"


namespace GNET
{

class AddCash_Re : public GNET::Protocol
{
	#include "addcash_re"

	void SendFailCash()
	{
		printf("AddCash_Re::SendFailCash: userid = %d \n", userid);
	}

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		if(retcode != ERR_SUCCESS ) 
		{ 
			SendFailCash(); return; 
		}
		MysqlManager::GetInstance()->AddCashLog(userid, zoneid, sn);
	}
};

};

#endif
