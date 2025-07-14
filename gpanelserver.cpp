
#include "panelchallenge.hpp"

#include "gpanelserver.hpp"
#include "state.hxx"

namespace GNET
{

GPanelServer GPanelServer::instance;

const Protocol::Manager::Session::State* GPanelServer::GetInitState() const
{
	return &state_GPanelLogin;
}

void GPanelServer::OnSetTransport(Session::ID sid, const SockAddr& local, const SockAddr& peer)
{
	Iterator it = sessions.find(sid);
	if (it != sessions.end())
	{
		it->second.SetLocal(local);
		it->second.SetPeer(peer);
	}
}

void GPanelServer::OnAddSession(Session::ID sid)
{
	//TODO
	printf("GPanelServer::OnAddSession %d \n", sid );
	
	int lk_rand[] = { rand() , rand() , rand() , rand() };
	Octets lk_key;
	lk_key.replace(lk_rand , sizeof(lk_rand));

	PanelChallenge pc(lk_key);
	sessions[sid].rand_key = lk_key;
	Send(sid, pc);
}

void GPanelServer::OnDelSession(Session::ID sid)
{
	//TODO
	Iterator it=sessions.find(sid);
	if(it==sessions.end())
		return;
	
	sessions.erase(sid);
	printf("GPanelServer::OnDelSession %d \n", sid );
}

};
