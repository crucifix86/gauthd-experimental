//---------------------------------------------------------------------------------------------------------------------------
//--PW LUA SCRIPT GDELIVERYD (C) DeadRaky 2022
//---------------------------------------------------------------------------------------------------------------------------
//#include <utf.h>
#include "rpcdefs.h"

#include "luaman.hpp"
#include <LuaBridge.h>
using namespace luabridge;

namespace GNET
{ 
	LuaManager* LuaManager::instance = 0;
	lua_State* LuaManager::L;
	pthread_mutex_t LuaManager::lua_mutex;
	const char * LuaManager::FName;
	time_t LuaManager::reload_tm = 0;
	size_t LuaManager::tick = 0;
	bool   LuaManager::lua_debug = false;
	LuaManager::CONFIG LuaManager::config;
	
	time_t LuaManager::GetFileTime(const char *path)
	{
		struct stat statbuf;
		if (stat(path, &statbuf) == -1) {
			perror(path);
			exit(1);
		}
		return statbuf.st_mtime;
	}

	bool LuaManager::IsTrue(int it, int * table)
	{
		for (unsigned int i = 0; table[i] && i < 128 ; i++)
			if (table[i] == it)
				return true;
		return false;
	}

	bool LuaManager::GetNum(const char* s, double& v)
	{
		LuaRef out = getGlobal(L, s);
		if (!out.isNil() && out.isNumber())
		{
			v = out;
			return true;
		}
		printf("LuaManager::GET_NUM: error %s not found! \n", s);
		return false;
	}

	bool LuaManager::GetString(const char* s, const char* v)
	{
		LuaRef out = getGlobal(L, s);
		if (!out.isNil() && out.isString())
		{
			v = out;
			return true;
		}
		printf("LuaManager::GET_STR: error %s not found! \n", s);
		return false;
	}

	bool LuaManager::SetNum(const char* s, double v)
	{
		LuaRef out = getGlobal(L, s);
		if (!out.isNil() && out.isNumber())
		{
			setGlobal(L, v, s);
			return true;
		}
		printf("LuaManager::SET_NUM: error %s not found! \n", s);
		return false;
	}

	bool LuaManager::SetString(const char* s, const char* v)
	{
		LuaRef out = getGlobal(L, s);
		if (!out.isNil() && out.isString())
		{
			setGlobal(L, v, s);
			return true;
		}
		printf("LuaManager::SET_STR: error %s not found! \n", s);
		return false;
	}

	bool LuaManager::SetAddr(const char* s, long long v)
	{
		LuaRef out = getGlobal(L, s);
		if (!out.isNil() && out.isNumber())
		{
			setGlobal(L, v, s);
			return true;
		}
		printf("LuaManager::SET_ADDR: error %s not found! \n", s);
		return false;
	}

	void LuaManager::game__Patch(long long address, int type, double value)
	{
		if (address)
		{
			switch (type)
			{
			case TYPE_CHAR: { *(char*)address = value; return; break; }
			case TYPE_SHORT: { *(short*)address = value; return; break; }
			case TYPE_INT: { *(int*)address = value; return; break; }
			case TYPE_INT64: { *(long long*)address = value; return; break; }
			case TYPE_FLOAT: { *(float*)address = value; return; break; }
			case TYPE_DOUBLE: { *(double*)address = value; return; break; }
			default: { printf("game__Patch: ERROR TYPE %d ! \n", type); return; break; }
			}
		}
	}

	double LuaManager::game__Get(long long address, int type, long long offset)
	{
		if (address)
		{
			switch (type)
			{
			case TYPE_CHAR: { return *(char*)(&((char*)address)[offset]); break; }
			case TYPE_SHORT: { return *(short*)(&((char*)address)[offset]); break; }
			case TYPE_INT: { return *(int*)(&((char*)address)[offset]); break; }
			case TYPE_INT64: { return *(long long*)(&((char*)address)[offset]); break; }
			case TYPE_FLOAT: { return *(float*)(&((char*)address)[offset]); break; }
			case TYPE_DOUBLE: { return *(double*)(&((char*)address)[offset]); break; }
			default: { printf("game__Get: ERROR TYPE %d ! \n", type);   return 0; break; }
			}
		}
		return 0;
	}

	void LuaManager::FunctionsRegister()
	{
		getGlobalNamespace(L).addFunction("game__Patch",game__Patch);
		getGlobalNamespace(L).addFunction("game__Get",game__Get);
	}

	void LuaManager::SetItem()
	{
		//config.SET_ADDR("GUILD_ROLE_RANK",config.GUILD_ROLE_RANK);
	}

	void LuaManager::GetItem()
	{
		double res = -1;
		GetNum( "lua_debug"				, res ) ? lua_debug			 			= res : res == -1;	
	}

	void LuaManager::FunctionsExec()
	{
		config.INIT();
	}

	#define LUA_MUTEX_BEGIN pthread_mutex_lock(&lua_mutex); \
							try { 
							
	#define LUA_MUTEX_END	} \
							catch (...) { printf("LUA::PANIC::EXCEPTION: ERROR \n"); } \
							pthread_mutex_unlock(&lua_mutex);

	void LuaManager::Init()
	{
		tick = 0;
		FName = "script.lua";
		pthread_mutex_init(&lua_mutex,0);
		LUA_MUTEX_BEGIN
		reload_tm = GetFileTime(FName);
		L = luaL_newstate();
		luaL_openlibs(L);
		FunctionsRegister();
		FunctionsExec();
		luaL_dofile(L, FName);
		SetItem();
		
		static const char * EventName = "Init";
		LuaRef Event = getGlobal(L, EventName);
		if(L && !Event.isNil() && Event.isFunction() )
		{
			if (lua_debug) printf("LUA::Event::LOG: Event = %s \n", EventName);
			Event();
		}
		else 
		{
			printf("LUA::Event: NULL!!! %s \n", EventName);
		}
		
		GetItem();
		LUA_MUTEX_END
	}

	void LuaManager::Update()
	{
		time_t last_tm = GetFileTime(FName);
		if(reload_tm == last_tm) return;	
		reload_tm = last_tm;
		LUA_MUTEX_BEGIN
		lua_close(L);
		L = luaL_newstate();
		luaL_openlibs(L);
		FunctionsRegister();
		FunctionsExec();
		luaL_dofile(L, FName);
		SetItem();
		static const char * EventName = "Update";
		LuaRef Event = getGlobal(L, EventName);
		if(L && !Event.isNil() && Event.isFunction() )
		{
			if (lua_debug) printf("LUA::Event::LOG: Event = %s \n", EventName);
			Event();
		}
		else 
		{
			printf("LUA::Event: NULL!!! %s \n", EventName);
		}
		GetItem();
		LUA_MUTEX_END
	}

	void LuaManager::HeartBeat()
	{
		LUA_MUTEX_BEGIN
		static const char * EventName = "HeartBeat";
		LuaRef Event = getGlobal(L, EventName);
		if(L && !Event.isNil() && Event.isFunction() )
		{
			//if (lua_debug) printf("LUA::Event::LOG: Event = %s , Tick = %d \n", EventName, tick);
			Event(tick);
		}
		else 
		{
			printf("LUA::Event: NULL!!! %s \n", EventName);
		}
		tick++;
		LUA_MUTEX_END
		
		if( !(tick % 30) )
		{
			Update();
		}
	}

	int LuaManager::EventOnUserLogin(int userid, const char * login, const char * ip, const char * hwid)
	{
		int res = 0;
		LUA_MUTEX_BEGIN
		static const char EventName[] = "EventOnUserLogin";
		LuaRef Event = getGlobal(L, EventName);
		if(L && !Event.isNil() && Event.isFunction() )
		{
			res = Event(userid, login, ip, hwid )[0];
		}
		else 
		{
			printf("LUA::Event: NULL!!! %s \n", EventName);
		}
		LUA_MUTEX_END
		return res;
	}

	void LuaTimer::UpdateTimer()
	{
		LuaManager::GetInstance()->HeartBeat();
	}

	void LuaTimer::Run()
	{
		UpdateTimer();
		Thread::HouseKeeper::AddTimerTask(this,update_time);
	}
};

