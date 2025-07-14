//---------------------------------------------------------------------------------------------------------------------------
//--PW LUA SCRIPT GDELIVERYD (C) DeadRaky 2022
//---------------------------------------------------------------------------------------------------------------------------
#ifndef __GNET_LUAMANAGER_H
#define __GNET_LUAMANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <mutex>
#include <sys/stat.h>
#include <sys/time.h>
extern "C" {
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
}

namespace GNET
{ 

	class LuaManager
	{
	public:
		enum
		{
			TYPE_CHAR 				= 0,
			TYPE_SHORT 				= 1,
			TYPE_INT 				= 2,
			TYPE_INT64 				= 3,
			TYPE_FLOAT 				= 4,
			TYPE_DOUBLE 			= 5,
		};
	private:
	#pragma pack(push, 1)
		struct CONFIG
		{
			void INIT() 
			{
				size_t i = 0;
			}
		};
	#pragma pack(pop)
	private:
		static lua_State* L;
		static pthread_mutex_t lua_mutex;
		static const char * FName;
		static time_t reload_tm;
		static size_t tick;
		static CONFIG config;
		static bool lua_debug;
		
	public:
		static void game__Patch(long long address, int type, double value);
		static double game__Get(long long address, int type, long long offset);
		
	public:	
		time_t GetFileTime(const char *path);
		bool GetNum(const char* s, double& v);
		bool GetString(const char* s, const char* v);
		bool SetNum(const char* s, double v);
		bool SetString(const char* s, const char* v);
		bool SetAddr(const char* s, long long v);
		bool IsTrue(int it, int * table);
		
		void FunctionsRegister();
		void FunctionsExec();
		void SetItem();
		void GetItem();
		void Init();
		void Update();
		void HeartBeat();

		int EventOnUserLogin(int userid, const char * login, const char * ip, const char * hwid);

	public:
		CONFIG * GetConfig() { return &config; }

	public:
		static LuaManager * GetInstance()
		{
			if (!instance)
			instance = new LuaManager();
			return instance;
		}
		static LuaManager * instance;
	};

	class LuaTimer : public Thread::Runnable
	{
		int update_time;
	public:
		LuaTimer(int _time,int _proir=1) : Runnable(_proir),update_time(_time) { }
		void Run();
	private:
		void UpdateTimer();
	};

};

#endif