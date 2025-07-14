#include <iostream>
#include <string>
#include <vector>
//#include <liblicense.h>

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"
#include "protocol.h"
#include "thread.h"
#include "timersender.h"
#include "octets.h"
#include "base64.h"
#include "getaddcashsn.hrp"
#include "addcash.hpp"

#include "authmanager.h"

using namespace GNET;

AuthManager* AuthManager::instance = NULL;

//-------------------------------------------------------------------------//

AuthManager::AuthManager()
{
	pthread_mutex_init(&antibrut_mutex,NULL);
	pthread_mutex_lock(&antibrut_mutex);
	antibrut_user.clear();
	pthread_mutex_unlock(&antibrut_mutex);
}

AuthManager::~AuthManager()
{
	pthread_mutex_lock(&antibrut_mutex);
	antibrut_user.clear();
	pthread_mutex_unlock(&antibrut_mutex);
	pthread_mutex_destroy(&antibrut_mutex);
}

void AuthManager::Init()
{
	pthread_mutex_lock(&antibrut_mutex);
	antibrut_user.clear();
	antibrut_user.reserve(6000);
	pthread_mutex_unlock(&antibrut_mutex);
}

void AuthManager::HeartBeat(size_t tick)
{
	if ( !(tick % 16) )
	{
		ClearAntibrut();
	}
}

void AuthManager::Auth0xMD5(Octets & in, Octets & out)
{
	out.resize(RESPONCE_SIZE);
	char * out_srt = (char*)out.begin();
	char * in_str = (char*)in.begin();
	
	int nt = 0;
	if(in_str[0] == '0' && in_str[1] == 'x') in_str += 2;
	for(int i = 0; i < RESPONCE_SIZE; ++i)
	{
		sscanf(&in_str[i*2], "%02x", &nt);
		out_srt[i] = (char)nt;
	}
	
}

void AuthManager::AuthBase64(Octets & in, Octets & out)
{
	Base64Decoder::Convert(out, in);
	out.resize(RESPONCE_SIZE);
}

bool AuthManager::AuthPasswd(Octets & in, Octets & out)
{
	bool res = false;
	pthread_mutex_lock(&antibrut_mutex);
	switch (MysqlManager::GetInstance()->GetHash())
	{
	case 1: //binary
		out = in; 
		res = true;
		break;
	case 2: //0xMD5
		Auth0xMD5(in,out);
		res = true;
		break;
	case 3: //base64
		AuthBase64(in,out);
		res = true;
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&antibrut_mutex);
	return res;
}

bool AuthManager::ValidLogin(Octets & in)
{
	bool res = true;
	pthread_mutex_lock(&antibrut_mutex);
	char * login = (char*)in.begin();
	for (size_t i = 0; i < in.size() && i < IDENTIFY_MAX_SIZE; i++)
	{
		if( !(login[i] >= '0' && login[i] <= '9') &&
			!(login[i] >= 'A' && login[i] <= 'Z') &&
			!(login[i] >= 'a' && login[i] <= 'z') &&
			!(login[i] == '_' || login[i] == '=')  )
		res = false;
	}
	pthread_mutex_unlock(&antibrut_mutex);
	return res;
}

void AuthManager::ClearAntibrut()
{
	pthread_mutex_lock(&antibrut_mutex);
	antibrut_user.clear();
	antibrut_user.reserve(6000);
	pthread_mutex_unlock(&antibrut_mutex);
}

int AuthManager::SizeAntibrut()
{
	int res = 0;
	pthread_mutex_lock(&antibrut_mutex);
	res = antibrut_user.size();
	pthread_mutex_unlock(&antibrut_mutex);
	return res;
}

int AuthManager::GetAntibrut(int ip)
{
	int res = -1;
	pthread_mutex_lock(&antibrut_mutex);
	for (unsigned int i = 0; i < antibrut_user.size(); i++)
	if  (antibrut_user.at(i).ip == ip) res = i;
	pthread_mutex_unlock(&antibrut_mutex);
	return res;
}

int AuthManager::DelAntibrut(int ip)
{
	int idx = GetAntibrut(ip);
	pthread_mutex_lock(&antibrut_mutex);
	if( idx >= 0)
	antibrut_user.erase(antibrut_user.begin() + idx);
	pthread_mutex_unlock(&antibrut_mutex);
	return idx;
}	

int AuthManager::AddAntibrut(int ip)
{
	int res = 0;
	int idx = GetAntibrut(ip);
	pthread_mutex_lock(&antibrut_mutex);
	if( idx >= 0)
	res = ++antibrut_user.at(idx).count;
	else
	antibrut_user.push_back(ANTIBRUT_USER(ip));
	pthread_mutex_unlock(&antibrut_mutex);
	return res;
}

//-------------------------------------------------------------------------//

void AuthTimer::UpdateTimer()
{
	static size_t tick = 0;
	tick++;
	MysqlManager::GetInstance()->HeartBeat(tick);
	AuthManager::GetInstance()->HeartBeat(tick);
}

void AuthTimer::Run()
{
	UpdateTimer();
	Thread::HouseKeeper::AddTimerTask(this,update_time);
}