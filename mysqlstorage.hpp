
#ifndef __GNET_MYSQLSTORAGE_HPP
#define __GNET_MYSQLSTORAGE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"
#include "gsql"

#include "mysqlstorage_re.hpp"
#include "gmysqlclient.hpp"
#include "gpanelserver.hpp"

namespace GNET
{

class MySQLStorage : public GNET::Protocol
{
	#include "mysqlstorage"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		MysqlManager *db = MysqlManager::GetInstance();
		GPanelServer * gps = GPanelServer::GetInstance();
		SessionInfo * sinfo = gps->GetSessionInfo(sid);
		if ( !sinfo )
		{
			printf("MySQLStorage::Process: ERR !sinfo \n");
			gps->Close(sid);
			return;
		}
		
		if (sinfo->panel_state != SessionInfo::STATE_GAME)
		{
			printf("MySQLStorage::Process: sinfo->panel_state != SessionInfo::STATE_GAME \n");
			gps->Close(sid);
			return;
		}
		
		MySQLStorage_Re MySQL_Re(id, userid, roleid, -1);
		GSQL iSQL(id, userid, roleid);
		iSQL.istr = input_str;
		
		if ( db->PanelQuery(iSQL) )
		{
			MySQL_Re.output_str = iSQL.ostr;
			MySQL_Re.retcode = ERR_SUCCESS;
		}
		else
		{
			MySQL_Re.retcode = -1;
		}
		
		GPanelServer::GetInstance()->Send(sid, MySQL_Re);
		return;
	}
};

};

#endif
