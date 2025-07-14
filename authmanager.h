#ifndef __GNET_AUTH_MANAGER_H
#define __GNET_AUTH_MANAGER_H

#include <iostream>
#include <string>
#include <vector>

#include "thread.h"
#include "octets.h"
#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"

namespace GNET
{ 

//-------------------------------------------------------------------------//

	class AuthManager
	{
	private:
		enum : size_t 
		{
			IDENTIFY_MAX_SIZE = 64,
			RESPONCE_SIZE = 16,
		};
	private:
		struct ANTIBRUT_USER
		{
			int ip;
			int count;
			ANTIBRUT_USER(int ip)
			{
				this->ip = ip;
				this->count = 0;
			}
		};
	private:
		pthread_mutex_t antibrut_mutex;
		std::vector <ANTIBRUT_USER> antibrut_user;
		
	public:
		AuthManager();
		~AuthManager();
	
		void Init();
		void HeartBeat(size_t tick);
		void Auth0xMD5 (Octets & in, Octets & out);
		void AuthBase64(Octets & in, Octets & out);
		bool AuthPasswd(Octets & in, Octets & out);
		bool ValidLogin(Octets & in);
		
		void ClearAntibrut();
		int SizeAntibrut();
		int GetAntibrut(int ip);
		int DelAntibrut(int ip);
		int AddAntibrut(int ip);
		
	public:
		static AuthManager * GetInstance()
		{
			if (!instance)
			instance = new AuthManager();
			return instance;
		}
		static AuthManager * instance;
	};

//-------------------------------------------------------------------------//

	class AuthTimer : public Thread::Runnable
	{
		int update_time;
	public:
		AuthTimer(int _time,int _proir=1) : Runnable(_proir),update_time(_time) { }
		void Run();
	private:
		void UpdateTimer();
	};	
	
};

#endif

