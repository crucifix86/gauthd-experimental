
#include "gauthserver.hpp"
#include "state.hxx"

namespace GNET
{

GAuthServer GAuthServer::instance;

const Protocol::Manager::Session::State* GAuthServer::GetInitState() const
{
	return &state_GAuthServer;
}

void GAuthServer::OnAddSession(Session::ID sid)
{
	//TODO
	printf("GAuthServer::OnAddSession %d \n", sid );
}

void GAuthServer::OnDelSession(Session::ID sid)
{
	//TODO
	Thread::Mutex::Scoped l(locker_zonemap);
	zonemap.erase(sid);
	printf("GAuthServer::OnDelSession %d \n", sid );
}

bool GAuthServer::ValidUser(Session::ID sid,int userid)
{
	printf("GAuthServer::ValidUser userid=%d \n",userid);
	Thread::RWLock::RDScoped l(locker_map);
	UserMap::iterator it=usermap.find(userid);
	if (it==usermap.end()) return false;
	if ((*it).second.sid != sid) return false;	
	return true;
}
/*

//------------------------------------------------------------------------
//--GOLD CHECKER--
//------------------------------------------------------------------------
void CheckTimer::CheckAddCashcn()
{
	GMysqlClient *db  = GMysqlClient::GetInstance();
	GAuthServer  *aum =  GAuthServer::GetInstance();
	if (db->GetCashSize() > 0) 
	{
		if ( !(clear_time++ % 3) )
		db->ClearCash();
		db->Autoclean();
		return;
	}
	int sid = aum->GetSidIdx();
	int count = 0;
	db->GetUseCashNow(0,count);
	for( int i = 0; i < count ; i++)
	{
		int userid = 0;
		int zoneid = 0;
		db->GetAllCashUser(i, userid, zoneid);
		if (userid > 0)
		{
			GetAddCashSN CashSN = GetAddCashSN(514, NULL , NULL );
			GetAddCashSNArg arg;
			arg.userid = userid;
			arg.zoneid = zoneid;
			CashSN.SetArgument(arg);
			CashSN.SetRequest();
			CashSN.Process(aum,sid);
		}else{
			db->DelCashUser(i);
		}
	}
}
//------------------------------------------------------------------------
void CheckTimer::AntibrutClear()
{
	GMysqlClient *db  = GMysqlClient::GetInstance();
	if(db->SizeAntibrutUser())
	db->ClearAntibrutUser();
}
//------------------------------------------------------------------------
void CheckTimer::Run()
{
	AntibrutClear();
	CheckAddCashcn();
	Thread::HouseKeeper::AddTimerTask(this,update_time);
}
//------------------------------------------------------------------------
*/

};
