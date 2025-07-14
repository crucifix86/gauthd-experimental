#ifndef __GNET_GPANELSERVER_HPP
#define __GNET_GPANELSERVER_HPP

#include <set>
#include <unordered_map>
#include "protocol.h"

namespace GNET
{

struct SessionInfo
{
	enum : size_t 
	{
		STATE_LOGIN = 0,
		STATE_GAME = 1,
	};
	
	int panel_state;
	Octets rand_key;
	SockAddr local;
	SockAddr peer;
	
	SessionInfo() : panel_state(STATE_LOGIN), rand_key(0), local(0), peer(0)
	{
	}
	
	void SetLocal(SockAddr addr) { local = addr;}
	void SetPeer(SockAddr addr) { peer = addr;}
	SockAddr GetLocal() const { return local;}
	SockAddr GetPeer() const { return peer;}
	const struct in_addr& GetPeerAddr() const {  return ((const struct sockaddr_in*)peer)->sin_addr; }
};

class GPanelServer : public Protocol::Manager
{
	typedef std::unordered_map<Session::ID, SessionInfo>	SessionInfoMap;
	typedef SessionInfoMap::iterator		Iterator;
	
	SessionInfoMap	sessions;
	
	static GPanelServer instance;
	size_t		accumulate_limit;
	const Session::State *GetInitState() const;
	bool OnCheckAccumulate(size_t size) const { return accumulate_limit == 0 || size < accumulate_limit; }
	void OnAddSession(Session::ID sid);
	void OnDelSession(Session::ID sid);
	void OnSetTransport(Session::ID sid, const SockAddr& local, const SockAddr& peer);
public:

	static bool ValidSid(Session::ID sid)
	{
		return instance.sessions.find(sid) != instance.sessions.end();
	}

	SessionInfo * GetSessionInfo(Session::ID sid)
	{
		Iterator it = sessions.find(sid);
		if (it != sessions.end())
			return &(it->second);
		return NULL;
	}

	static GPanelServer *GetInstance() { return &instance; }
	std::string Identification() const { return "GPanelServer"; }
	void SetAccumulate(size_t size) { accumulate_limit = size; }
	GPanelServer() : accumulate_limit(0) { }
	Octets shared_key;
	Octets client_key;
};

};
#endif
