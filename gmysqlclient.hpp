#ifndef __GNET_GMYSQLCLIENT_HPP
#define __GNET_GMYSQLCLIENT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <mysql/mysql.h>

#include "thread.h"
#include "octets.h"
#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"

#include "gusecashnow"
#include "gsql"

// database format
#include "ec_sqlarenaplayer"
#include "ec_sqlarenateam"


#include "ec_arenateam"
#include "ec_arenaplayer"
#include "ec_arenateamsync"
#include "ec_arenaplayersync"
#include "ec_arenateamtoplist"
#include "ec_arenaplayertoplist"

#define MYSQL_MUTEX_BEGIN Lock(); \
							try \
							{ 
								//TODO 
#define MYSQL_MUTEX_END		} \
							catch (...) \
							{ \
								printf("MYSQL::PANIC::EXCEPTION: ERROR \n"); \
							} \
							Unlock();


namespace GNET
{ 
	class MysqlManager
	{
	private:
		enum : size_t
		{
			IP_LEN = 20u,
			LEN__ROW = 2048u,
			LEN__INPUT = 512u,
			LEN__OUTPUT = 512u,
			LEN__QUERY = 1024u,
			
			DEFAULT_ARENA_SCORE = 1500,
			MAX_ARENA_PLAYERS = 6,
			MAX_TEAM_MODE = 2,
		};
	private:
	
		unsigned int sid;
		int zoneid;
		int aid;
	
		bool active;
		MYSQL * DBMysql;
		pthread_mutex_t mysql_mutex;
		pthread_mutex_t query_mutex;
		
		std::string address;
		unsigned short port;
		std::string user;
		std::string passwd;
		std::string dbname;
		int hash;
		
		GUseCashNowVector usecashnow;
		
	public:
		MysqlManager();
		~MysqlManager();
		
		void Init(const char * _ip, const short _port, const char * _user, const char * _passwd, const char * _db, int _hash);
		void HeartBeat(size_t tick);
		void Lock();
		void Unlock();
		
		bool Connect();
		bool Disconnect();
		bool Reconnect();
		bool CheckFailedExcept();
		bool MysqlQuery(GSQL & iSQL, size_t iPos);
		bool MysqlSender(GSQL & iSQL);
		int  GetIndexPos(GSQL & iSQL, size_t iPos);
		
		Octets & GetRowOct(GSQL & iSQL, size_t iPos, size_t rPos);
		const char * GetRowStr(GSQL & iSQL, size_t iPos, size_t rPos);
		size_t GetRowNum(GSQL & iSQL, size_t iPos, size_t rPos);
		double GetRowFlt(GSQL & iSQL, size_t iPos, size_t rPos);
		
		
		bool MatrixPasswd(int &uid, Octets & identify, Octets & responce);
		
		// New bcrypt authentication methods
		bool MatrixPasswdBcrypt(int &uid, const std::string& username, std::string& stored_hash);
		bool UpdatePasswordToBcrypt(int uid, const std::string& bcrypt_hash);
		bool CheckPasswordFormat(int uid, int& hash_type); // Returns: 1=MD5, 2=Base64, 3=Binary, 4=Bcrypt
		
		bool ClearOnlineRecord(unsigned int sid, int zid, int aid);
		bool OnlineRecord(int uid, int &zid, int &zonelocalid, int &overwrite);
		bool OnfflineRecord(int uid, int &zonelocalid, int &overwrite);
		bool UserCreatime(int uid, int &timestamp);
		bool UserGMPrivilege(int uid, int zid, bool &IsGM);
		bool QueryGMPrivilege(int uid, int zid, ByteVector & auth);

		bool ClearUseCash();
		bool GiveUseCash();
		bool UpdateUseCashSN(int uid, int zid, int sn, int & cash);
		bool AddCashLog(int uid, int zid, int sn);
		
		// ec arena
		bool GetName(Octets & name_base64, Octets & name);
		bool SetName(Octets & name_base64, Octets & name);
		bool CheckName(Octets & name);
		
		bool SetIntVec(std::string & sqlvec, IntVector & vec);
		bool GetIntVec(Octets & sqltext, IntVector & vec);
		bool SetRoleVec(std::string & sqlvec, EC_SQLArenaTeamMemberVector & vec);
		bool GetRoleVec(Octets & sqltext, EC_SQLArenaTeamMemberVector & vec);
		bool GetRoleTopVec(Octets & sqltext, Int64Vector & vec);
		bool CheckValidVec(Octets & sqltext);
		
		bool MakeDefaultPlayer(EC_SQLArenaPlayer & pPlayer, int roleid,  int cls, Octets & name);
		bool MakeDefaultTeam(EC_SQLArenaTeam & pTeam, int capitan_id, int team_type, Octets & team_name);
		
		int GetArenaPlayer( int roleid, EC_SQLArenaPlayer & pPlayer );
		int SetArenaPlayer( int roleid, EC_SQLArenaPlayer & pPlayer );
		int CreateArenaPlayer(int roleid, int cls, Octets & name, EC_SQLArenaPlayer & pPlayer );
		int DeleteArenaPlayer(int roleid );
		
		int GetArenaTeam( int team_id, int capitan_id, EC_SQLArenaTeam & pTeam );
		int SetArenaTeam( int team_id, int capitan_id, EC_SQLArenaTeam & pTeam );
		int CreateArenaTeam( int capitan_id, int team_type, Octets & team_name, EC_SQLArenaTeam & pTeam );
		int DeleteArenaTeam( int team_id );		

		int MakeArenaPlayerTopList( EC_ArenaPlayerTopListVector & vec );
		int MakeArenaTeamTopList( EC_ArenaTeamTopListVector & vec );

		// ec arena end
		
		bool PanelQuery(GSQL & iSQL);
		
		inline int GetHash() { return hash; }
	public:
		static MysqlManager * GetInstance()
		{
			if (!instance)
			instance = new MysqlManager();
			return instance;
		}
		static MysqlManager * instance;
	};

	//-----------------------------------------------------------------------

	class MysqlTimer : public Thread::Runnable
	{
		int update_time;
	public:
		MysqlTimer(int _time,int _proir=1) : Runnable(_proir),update_time(_time) { }
		void Run();
	private:
		void UpdateTimer();
	};

};

#endif