
#ifndef __GNET_PANELRESPONSE_HPP
#define __GNET_PANELRESPONSE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"

#include "panelresponse_re.hpp"
#include "gpanelserver.hpp"

namespace GNET
{

class PanelResponse : public GNET::Protocol
{
	#include "panelresponse"

	void Send_Result(Manager::Session::ID sid, int result )
	{
		PanelResponse_Re pr_re(result);
		GPanelServer::GetInstance()->Send(sid, pr_re);
	}

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		GPanelServer * gps = GPanelServer::GetInstance();
		SessionInfo * sinfo = gps->GetSessionInfo(sid);
		if ( !sinfo )
		{
			printf("PanelResponse::Process: ERR !sinfo \n");
			Send_Result(sid,1);
			gps->Close(sid);
			return;
		}

		if ( nonce.size() != 16 )
		{
			printf("PanelResponse::Process: ERR nonce.size() != 16 \n");
			Send_Result(sid,2);
			gps->Close(sid);
			return;
		}
		
		Octets key = sinfo->rand_key;
		Octets client_key = gps->client_key;
		
		MD5Hash hmd5;
		HMAC_MD5Hash hash;
		
		hmd5.Digest(key);
		hash.SetParameter(key);
		hash.Update(client_key);
		hash.Final(key);
			
		if (key != nonce)
		{
			printf("PanelResponse::Process: ERR key != nonce \n");
			Send_Result(sid,3);
			gps->Close(sid);
			return;
		}
		
		Send_Result(sid,ERR_SUCCESS);
		sinfo->panel_state = SessionInfo::STATE_GAME;
		printf("PanelResponse::Process: OK!!! \n", sid);
		gps->ChangeState(sid,&state_GPanelServer);
		return;
	}
};

};

#endif
