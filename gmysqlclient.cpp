#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <mysql/mysql.h>
//#include <liblicense.h>
#include "../include/beaktrace.h"

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
#include "gmysqlclient.hpp"

using namespace GNET;

MysqlManager* MysqlManager::instance = NULL;

MysqlManager::MysqlManager()
{
	pthread_mutex_init(&mysql_mutex,NULL);
	pthread_mutex_lock(&mysql_mutex);
	
	pthread_mutex_init(&query_mutex,NULL);
	active = false;
	DBMysql = NULL;
	address.clear();
	port = 0;
	user.clear();
	passwd.clear();
	dbname.clear();
	hash = 0;
	
	pthread_mutex_unlock(&mysql_mutex);
}

MysqlManager::~MysqlManager()
{
	pthread_mutex_lock(&mysql_mutex);
	
	active = false;
	delete DBMysql; DBMysql = NULL;
	address.clear();
	user.clear();
	passwd.clear();
	dbname.clear();
	pthread_mutex_destroy(&query_mutex);
	
	pthread_mutex_unlock(&mysql_mutex);
	pthread_mutex_destroy(&mysql_mutex);
}

void MysqlManager::Init(const char * _ip, const short _port, const char * _user, const char * _passwd, const char * _db, int _hash)
{
	pthread_mutex_lock(&mysql_mutex);
	
	VM_BEGIN
	
	if ( LIC_INIT_MYSQL )
	{
		address = _ip;
		port = _port;
		user = _user;
		passwd = _passwd;
		dbname = _db;
		hash = _hash;
		active = false;
	}
	
	VM_END
	
	pthread_mutex_unlock(&mysql_mutex);
}

void MysqlManager::HeartBeat(size_t tick)
{
	if ( !(tick % 60) )
	{
		if ( GiveUseCash() )
		{
			ClearUseCash();
		}
	}
}

void MysqlManager::Lock()
{
	pthread_mutex_lock(&query_mutex);
}

void MysqlManager::Unlock()
{
	pthread_mutex_unlock(&query_mutex);
}


bool MysqlManager::Connect()
{
	bool res = false;
	pthread_mutex_lock(&mysql_mutex);
			
	DBMysql = mysql_init(NULL);
	if(DBMysql)
	{
		if( mysql_real_connect(DBMysql,address.c_str(),user.c_str(),passwd.c_str(),dbname.c_str(),port,0,0) && !CheckFailedExcept() )
		{
			std::cout <<  "MysqlManager: AddSession " << std::endl;
			active = true;
			res = true;
		}
	}
	
	pthread_mutex_unlock(&mysql_mutex);
	return res;
}

bool MysqlManager::Disconnect()
{
	bool res = false;
	pthread_mutex_lock(&mysql_mutex);
	if (DBMysql)
	{
		mysql_close(DBMysql);
		DBMysql = NULL;
		active = false;
		res = true;
		std::cout <<  "MysqlManager: DelSession " << std::endl;
	}
	pthread_mutex_unlock(&mysql_mutex);
	return res;
}

bool MysqlManager::Reconnect()
{
	bool res = false;
	pthread_mutex_lock(&mysql_mutex);
	
	if (DBMysql)
	{
		mysql_close(DBMysql);
		DBMysql = NULL;
		active = false;
	}
	
	DBMysql = mysql_init(NULL);
	if(DBMysql)
	{
		if( mysql_real_connect(DBMysql,address.c_str(),user.c_str(),passwd.c_str(),dbname.c_str(),port,0,0) && !CheckFailedExcept() )
		{
			active = true;
			res = true;
			std::cout <<  "MysqlManager: Reconnect --OK-- " << std::endl;
		}
		else
		{
			sleep(5);
			std::cout <<  "MysqlManager: Reconnecting ... " << std::endl;
			pthread_mutex_unlock(&mysql_mutex);
			Reconnect();
		}
	}
	
	pthread_mutex_unlock(&mysql_mutex);
	return res;
}

bool MysqlManager::CheckFailedExcept()
{
	bool res = false;
	if ( DBMysql )
	{
		if ( mysql_errno(DBMysql) )
		{
			active == false;
			const char * err = mysql_error(DBMysql);
			if (err)
			{
				std::cout <<  "MysqlManager: error: " << err << std::endl;
			}
			res = true;
		}
	}
	else
	{
		active == false;
		std::cout <<  "MysqlManager: error: MYSQL == NULL " << std::endl;
		res = true;
	}
	return res;
}

bool MysqlManager::MysqlQuery(GSQL & iSQL, size_t iPos)
{
	bool res = false;
	
	if ( DBMysql && active ) 
	{
		const char * str = (const char *)iSQL.istr[iPos].begin();
		size_t len = str && str[0] ? strlen(str) : 0;
		
		if (len && !mysql_real_query(DBMysql, str, len) && mysql_field_count(DBMysql) > 0 )
		{
			MYSQL_RES * oSQL = mysql_store_result(DBMysql);
			if (oSQL)
			{
				int oCount = mysql_num_rows(oSQL);
				
				if (oCount > LEN__OUTPUT)
				{
					oCount = LEN__OUTPUT;
				}
				
				iSQL.ostr.clear();
				iSQL.ostr.resize(oCount);
				
				for (size_t i = 0; i < iSQL.ostr.size() && i < LEN__OUTPUT; i++)
				{
					MYSQL_ROW row = mysql_fetch_row(oSQL);
					int rCount = mysql_num_fields(oSQL);
					
					iSQL.ostr[i].iindex = iPos;
					iSQL.ostr[i].str.clear();
					iSQL.ostr[i].str.resize(rCount);
					
					for (size_t j = 0; j < iSQL.ostr[i].str.size() && j < LEN__ROW; j++)
					{
						if ( row && row[j] && row[j][0] )
						{
							iSQL.ostr[i].str[j].replace( row[j], (strlen(row[j])+1) );
						}
						else
						{
							iSQL.ostr[i].str[j].replace( "NULL", 5 );
						}
					}
					
				}
				
				mysql_free_result(oSQL);
				res = true;
			}
		}
	}
	return res;
}

bool MysqlManager::MysqlSender(GSQL & iSQL)
{
	bool res = false;
	pthread_mutex_lock(&mysql_mutex);
	if(!CheckFailedExcept())
	{
		while ( mysql_next_result(DBMysql) == 0 );
		for (size_t i = 0; i < iSQL.istr.size() && i < LEN__INPUT; i++)
		{
			MysqlQuery(iSQL, i);
		}
		
		
		//LOGS BRGIN 
		printf("MYSQL::LOG:Input: INPUT_COUNT = %d \n", iSQL.istr.size() );
		for (size_t i = 0; i < iSQL.istr.size() && i < LEN__INPUT; i++)
		{
			std::string iLog((const char * )iSQL.istr[i].begin() , iSQL.istr[i].size() );
			printf("MYSQL::LOG:Input: iIndex = %d , str = %s \n", i, iLog.c_str() );
		}
		
		printf("MYSQL::LOG:Input: ONPUT_COUNT = %d \n", iSQL.ostr.size() );
		for (size_t i = 0; i < iSQL.ostr.size() && i < LEN__OUTPUT; i++)
		{
			printf("MYSQL::LOG:Input: ONPUT_POS = %d , ROW_COUNT = %d \n", i , iSQL.ostr[i].str.size() );
			for (size_t j = 0; j < iSQL.ostr[i].str.size() && j < LEN__ROW; j++)
			{
				std::string iLog((const char * )iSQL.ostr[i].str[j].begin() , iSQL.ostr[i].str[j].size() );
				printf("MYSQL::LOG:Output: iIndex = %d , oPos=%d , rPos=%d , str = %s \n", iSQL.ostr[i].iindex , i, j, iLog.c_str() );
			}
		}
		//LOGS END
		
		
		res = true;
	}
	pthread_mutex_unlock(&mysql_mutex);
	if (!res)
	{
		Reconnect();
		res = MysqlSender(iSQL);
	}
	return res;
}

int MysqlManager::GetIndexPos(GSQL & iSQL, size_t iPos)
{
	for (size_t i = 0; i < iSQL.ostr.size() && i < LEN__OUTPUT; i++)
	{
		if (iPos == iSQL.ostr[i].iindex)
		{
			return i;
		}
	}
	return 0;
}

Octets & MysqlManager::GetRowOct(GSQL & iSQL, size_t iPos, size_t rPos)
{
	static Octets res;
	if ( iPos < iSQL.ostr.size() && rPos < iSQL.ostr[iPos].str.size() && iSQL.ostr[iPos].str[rPos].size() > 1 )
	{
		//iSQL.ostr[iPos].str[rPos].resize(iSQL.ostr[iPos].str[rPos].size()-1);
		return iSQL.ostr[iPos].str[rPos];
	}
	printf("MysqlManager::GetRowOct: iPos=%d , rPos=%d \n", iPos, rPos);
	res.clear();
	return res;
}

const char * MysqlManager::GetRowStr(GSQL & iSQL, size_t iPos, size_t rPos)
{
	if ( iPos < iSQL.ostr.size() && rPos < iSQL.ostr[iPos].str.size() )
	{
		return (const char*)iSQL.ostr[iPos].str[rPos].begin();
	}
	printf("MysqlManager::GetRowStr: iPos=%d , rPos=%d , oSize=%d , rSize=%d \n", iPos, rPos, iSQL.ostr.size(), iSQL.ostr[iPos].str.size() );
	return NULL;
}

size_t MysqlManager::GetRowNum(GSQL & iSQL, size_t iPos, size_t rPos)
{
	const char * str = GetRowStr(iSQL, iPos, rPos);
	if (str && strlen(str) < 40)
	{
		return atoll(str);
	}
	printf("MysqlManager::GetRowNum: iPos=%d , rPos=%d , oSize=%d , rSize=%d \n", iPos, rPos, iSQL.ostr.size(), iSQL.ostr[iPos].str.size() );
	return 0;
}

double MysqlManager::GetRowFlt(GSQL & iSQL, size_t iPos, size_t rPos)
{
	const char * str = GetRowStr(iSQL, iPos, rPos);
	if (str && strlen(str) < 40)
	{
		return atof(str);
	}
	printf("MysqlManager::GetRowFlt: iPos=%d , rPos=%d , oSize=%d , rSize=%d \n", iPos, rPos, iSQL.ostr.size(), iSQL.ostr[rPos].str.size() );
	return 0.0f;
}

//-------------------[packets]--------------------

bool MysqlManager::MatrixPasswd(int &uid, Octets & identify, Octets & responce)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 2;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;		
	std::string login((const char*)identify.begin(), identify.size());
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"CALL acquireuserpasswd('%s', @userid, @passwd)",login.c_str());
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT @userid, @passwd");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 )
	{
		size_t iIndex = GetIndexPos(iSQL,1);
		
		if (iSQL.ostr[iIndex].str.size() >= 2)
		{
			uid = GetRowNum(iSQL, iIndex, 0);
			responce = GetRowOct(iSQL, iIndex, 1);
			responce.resize(responce.size()-1);
			
			if (uid > 0 && responce.size() > 0)
			{
				res = true;
			}
		}
	}

	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::MatrixPasswdBcrypt(int &uid, const std::string& username, std::string& stored_hash)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 2;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"CALL acquireuserpasswd('%s', @userid, @passwd)", username.c_str());
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT @userid, @passwd");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 )
	{
		size_t iIndex = GetIndexPos(iSQL,1);
		
		if (iSQL.ostr[iIndex].str.size() >= 2)
		{
			uid = GetRowNum(iSQL, iIndex, 0);
			const char* hash_str = GetRowStr(iSQL, iIndex, 1);
			if (hash_str) {
				stored_hash = std::string(hash_str);
				if (uid > 0 && !stored_hash.empty()) {
					res = true;
				}
			}
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::UpdatePasswordToBcrypt(int uid, const std::string& bcrypt_hash)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
		"UPDATE auth SET passwd = '%s', passwd_type = 4 WHERE userid = %d", bcrypt_hash.c_str(), uid);
	
	if ( MysqlSender(iSQL) )
	{
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::CheckPasswordFormat(int uid, int& hash_type)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
		"SELECT passwd_type FROM auth WHERE userid = %d", uid);
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 )
	{
		size_t iIndex = GetIndexPos(iSQL,0);
		if (iSQL.ostr[iIndex].str.size() >= 1)
		{
			hash_type = GetRowNum(iSQL, iIndex, 0);
			if (hash_type == 0) hash_type = 1; // Default to MD5 if not set
			res = true;
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::ClearOnlineRecord(unsigned int sid, int zid, int aid)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"CALL clearonlinerecords(%d, %d)", zid, aid);
	
	if ( MysqlSender(iSQL) )
	{
		this->sid = sid;
		this->zoneid = zid;
		this->aid = aid;
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::OnlineRecord(int uid, int &zid, int &zonelocalid, int &overwrite)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, uid, 0);
	size_t SqlCount = 3;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SET @zoneid = %d, @aid = %d, @zonelocalid = %d",zid,aid,zonelocalid);
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"CALL recordonline(%d, %d, @zoneid, @zonelocalid, @overwrite)",uid, aid);
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT @zoneid, @zonelocalid, @overwrite");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 )
	{
		size_t iIndex = GetIndexPos(iSQL,2);
		if (iSQL.ostr[iIndex].str.size() >= 3)
		{
			zid = GetRowNum(iSQL, iIndex, 0);
			zonelocalid = GetRowNum(iSQL, iIndex, 1);
			overwrite = GetRowNum(iSQL, iIndex, 2);
			res = true;
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::OnfflineRecord(int uid, int &zonelocalid, int &overwrite)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, uid, 0);
	size_t SqlCount = 3;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SET @zoneid = %d, @aid = %d, @zonelocalid = %d",zoneid,aid,zonelocalid);
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"CALL recordoffline(%d, %d, @zoneid, @zonelocalid, @overwrite)",uid, aid);
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT @zoneid, @zonelocalid, @overwrite");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 )
	{
		size_t iIndex = GetIndexPos(iSQL,2);
		if (iSQL.ostr[iIndex].str.size() >= 3)
		{
			//zid = GetRowNum(iSQL, iIndex, 0);
			zonelocalid = GetRowNum(iSQL, iIndex, 1);
			overwrite = GetRowNum(iSQL, iIndex, 2);
			res = true;
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::UserCreatime(int uid, int &timestamp)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, uid, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT UNIX_TIMESTAMP(creatime) FROM users WHERE ID=%d",uid);
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 && iSQL.ostr[0].str.size() >= 1 )
	{
		timestamp = GetRowNum(iSQL, 0, 0);
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::UserGMPrivilege(int uid, int zid, bool &IsGM)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, uid, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT rid FROM auth WHERE userid=%d AND zoneid=%d",uid,zid);
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 && iSQL.ostr[0].str.size() >= 1 )
	{
		IsGM = true;
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::QueryGMPrivilege(int uid, int zid, ByteVector & auth)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, uid, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT rid FROM auth WHERE userid=%d AND zoneid=%d",uid,zid);
	
	if ( MysqlSender(iSQL) )
	{
		for (size_t j = 0; j < iSQL.ostr.size() && j < LEN__OUTPUT; j++ )
		{
			if ( iSQL.ostr[j].str.size() >= 1 )
			{
				auth.add(GetRowNum(iSQL, j, 0));
			}
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

//---------------------------------------------------------------------------------

bool MysqlManager::ClearUseCash()
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"DELETE FROM usecashnow WHERE status=%d", 0);
	
	if ( MysqlSender(iSQL) )
	{
		res = true;
	}

	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::GiveUseCash()
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	GAuthServer * aum = GAuthServer::GetInstance();
	
	size_t i = 0;
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),"SELECT userid, zoneid, sn, aid, point, cash, status, UNIX_TIMESTAMP(creatime) FROM usecashnow WHERE status=%d",0);
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() )
	{
		size_t oSize = iSQL.ostr.size();
		
		if (oSize > LEN__OUTPUT)
		{
			oSize = LEN__OUTPUT;
		}
		
		usecashnow.clear();
		usecashnow.resize(oSize);
		
		for (size_t j = 0; j < oSize && j < LEN__OUTPUT; j++ )
		{
			if ( iSQL.ostr[j].str.size() >= 8 )
			{
				usecashnow[j].userid   = GetRowNum(iSQL, j, 0);
				usecashnow[j].zoneid   = GetRowNum(iSQL, j, 1);
				usecashnow[j].sn       = GetRowNum(iSQL, j, 2);
				usecashnow[j].aid      = GetRowNum(iSQL, j, 3);
				usecashnow[j].point    = GetRowNum(iSQL, j, 4);
				usecashnow[j].cash     = GetRowNum(iSQL, j, 5);
				usecashnow[j].status   = GetRowNum(iSQL, j, 6);
				usecashnow[j].creatime = GetRowNum(iSQL, j, 7);
				
				int _uid = usecashnow[j].userid;
				int _zid = usecashnow[j].zoneid;
				int _cash = usecashnow[j].cash;
				
				if (_uid > 0 && _cash && _zid == zoneid )
				{
					GetAddCashSN CashSN = GetAddCashSN(RPC_GETADDCASHSN, NULL , NULL );
					GetAddCashSNArg arg;
					arg.userid = _uid;
					arg.zoneid = _zid;
					CashSN.SetArgument(arg);
					// CashSN.SetResult(); // Need proper result object
					CashSN.Process(aum,sid);
				}
			}
		}
		
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::UpdateUseCashSN(int uid, int zid, int sn, int & cash)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	for (size_t j = 0; j < usecashnow.size() && j < LEN__OUTPUT; j++ )
	{
		if (usecashnow[j].userid == uid && usecashnow[j].zoneid == zid && usecashnow[j].cash && usecashnow[j].status == 0)
		{
			usecashnow[j].status = 1;
			usecashnow[j].sn = sn;
			cash = usecashnow[j].cash;
			res = true;
			break;
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

bool MysqlManager::AddCashLog(int uid, int zid, int sn)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	for (size_t j = 0; j < usecashnow.size() && j < LEN__OUTPUT; j++ )
	{
		if (usecashnow[j].userid == uid && usecashnow[j].zoneid == zid && usecashnow[j].status == 1 && usecashnow[j].sn == sn)
		{
			size_t i = 0;
			GSQL iSQL(0, uid, 0);
			size_t SqlCount = 1;
			iSQL.istr.resize(SqlCount);

			sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
				"REPLACE INTO usecashlog VALUES (%d, %d, %d, %d, %d, %d, %d, creatime=FROM_UNIXTIME(%d), NOW())", 
				usecashnow[j].userid,
				usecashnow[j].zoneid,
				usecashnow[j].sn,
				usecashnow[j].aid,
				usecashnow[j].point,
				usecashnow[j].cash,
				usecashnow[j].status,
				usecashnow[j].creatime
			);
			
			if ( MysqlSender(iSQL) )
			{
				res = true;
			}
			
			//printf("MysqlManager::AddCashLog: pos=%d \n", j);
			
			break;
		}
	}
	
	MYSQL_MUTEX_END
	return res;
}

//-----------------------------------------------------------------------------//

bool MysqlManager::GetName(Octets & name_base64, Octets & name)
{
	bool res = false;
	
	if ( name_base64.size() > 1 )
	{
		name_base64.resize(name_base64.size()-1);
		Base64Decoder base64_decoder;
		name = base64_decoder.Update(name_base64);
		res = true;
	}
	
	//printf("MysqlManager::GetName: name_base64='%.*s' , size_base64='%d' name='%.*s' , size='%d' result='%d' \n", name_base64.size(), name_base64.begin(), name_base64.size(), name.size(), name.begin(), name.size(), res);
	return res;
}

bool MysqlManager::SetName(Octets & name_base64, Octets & name)
{
	bool res = false;
	
	if(name.size() > 1) 
	{
		
		Base64Encoder::Convert(name_base64, name);
		res = true;
	}
	
	//printf("MysqlManager::SetName: name_base64='%.*s' , size_base64='%d' name='%.*s' , size='%d' result='%d' \n", name_base64.size(), name_base64.begin(), name_base64.size(), name.size(), name.begin(), name.size(), res);
	return res;
}

bool MysqlManager::CheckName(Octets & name)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	if ( name.size() > 1 )
	{
		Octets name_base64;
		Base64Encoder::Convert(name_base64, name);
			
		size_t i = 0;
		GSQL iSQL(0, 0, 0);
		size_t SqlCount = 1;
		iSQL.istr.resize(SqlCount);
		
		sprintf((char*)iSQL.istr[i++].resize( LEN__QUERY).begin(), "SELECT * FROM `arena_teams` WHERE name = '%s' ", name_base64.begin() );
		
		if ( MysqlSender(iSQL) && iSQL.ostr.size() )
		{
			res = true;
		}
	}

	MYSQL_MUTEX_END
	return res;
}

//-----------------------------------------------------------------------------//

bool MysqlManager::SetIntVec(std::string & sqlvec, IntVector & vec)
{
	bool res = false;
	//printf("MysqlManager::SetIntVec: begin \n");
	
	sqlvec.clear();
	for ( size_t i = 0; i < vec.size() && i < 128; ++i )
	{
		sqlvec += std::to_string( vec[i] ) + ",";
	}
	
	//printf("MysqlManager::SetIntVec: end \n");
	return res;
}

bool MysqlManager::GetIntVec(Octets & sqltext, IntVector & vec)
{
	bool res = false;
	//printf("MysqlManager::GetIntVec: begin \n");
	vec.clear();
	try 
	{
		if ( CheckValidVec(sqltext) )
		{
			size_t count = 0;
			std::string sqlvec( (const char*)sqltext.begin() , sqltext.size()-1 );
	
			if ( sqlvec.find(',') != std::string::npos )
			{
				std::stringstream ss(sqlvec);
				std::string s;
				
				for (char c : sqlvec) 
				{
					c == ',' ? count++ : count;
				}
				
				if ( count )
				{
					for (size_t i = 0; i < count && i < 6; i++ )
					{
						std::getline(ss, s, ',');
						if ( s.size() > 1 && s.size() < 12 )
						{
							int item = 0;
							sscanf(s.c_str(), "%d", &item);
							vec.push_back(item);
						}
						else
						{
							break;
						}
					}
					
					res = true;
				}
			}
		}
    }
    catch ( ... ) 
    {
        std::cout << "GetIntVec:ERR: " << std::endl;
    }
	//printf("MysqlManager::GetIntVec: end \n");
	return res;
}

bool MysqlManager::SetRoleVec(std::string & sqlvec, EC_SQLArenaTeamMemberVector & vec)
{
	bool res = false;
	//printf("MysqlManager::SetRoleVec: begin \n");
	sqlvec.clear();
	for ( size_t i = 0; i < vec.size() && i < 128; ++i )
	{
		sqlvec += std::to_string(vec[i].player_id) + ":" + std::to_string(vec[i].award_count) + ",";
	}
	//printf("MysqlManager::SetRoleVec: end \n");
	return res;
}

bool MysqlManager::GetRoleVec(Octets & sqltext, EC_SQLArenaTeamMemberVector & vec)
{
	bool res = false;
	//printf("MysqlManager::GetRoleVec: begin \n");
	vec.clear();
	try 
	{
		if ( CheckValidVec(sqltext) )
		{
			size_t player_count = 0;
			size_t award_count = 0;
			std::string sqlvec( (const char*)sqltext.begin() , sqltext.size()-1 );
	
			if ( sqlvec.find(',') != std::string::npos && sqlvec.find(':') != std::string::npos )
			{
				std::stringstream ss(sqlvec);
				std::string s;
				
				for (char c : sqlvec) 
				{
					c == ',' ? player_count++ : c == ':' ? award_count++ : award_count;
				}
				
				if ( player_count > 0 && player_count == award_count )
				{
					for (size_t i = 0; i < player_count && i < 6; i++ )
					{
						std::getline(ss, s, ',');
						if ( s.size() > 2 && s.size() < 24 && s.find(':') != std::string::npos )
						{
							int roleid = 0;
							int award = 0;
							sscanf(s.c_str(), "%d:%d", &roleid, &award);
							printf("MysqlManager::GetRoleVec: roleid=%d , award=%d \n", roleid, award);
							vec.push_back(EC_SQLArenaTeamMember(roleid, award));
						}
						else
						{
							break;
						}
					}
					
					res = true;
				}
			}
		}
    }
    catch ( ... ) 
    {
        std::cout << "GetRoleVec:ERR: " << std::endl;
    }
	//printf("MysqlManager::GetRoleTopVec: end \n");
	return res;
}

bool MysqlManager::GetRoleTopVec(Octets & sqltext, Int64Vector & vec)
{
	bool res = false;
	//printf("MysqlManager::GetRoleTopVec: begin \n");
	vec.clear();
	try 
	{
		if ( CheckValidVec(sqltext) )
		{
			size_t player_count = 0;
			size_t award_count = 0;
			std::string sqlvec( (const char*)sqltext.begin() , sqltext.size()-1 );
	
			if ( sqlvec.find(',') != std::string::npos && sqlvec.find(':') != std::string::npos )
			{
				std::stringstream ss(sqlvec);
				std::string s;
				
				for (char c : sqlvec) 
				{
					c == ',' ? player_count++ : c == ':' ? award_count++ : award_count;
				}
				
				if ( player_count > 0 && player_count == award_count  )
				{
					for (size_t i = 0; i < player_count && i < 6; i++ )
					{
						std::getline(ss, s, ',');
						if ( s.size() > 2 && s.size() < 24 && s.find(':') != std::string::npos )
						{
							int roleid = 0;
							int award = 0;
							sscanf(s.c_str(), "%d:%d", &roleid, &award);
							vec.push_back(roleid);
						}
						else
						{
							break;
						}
					}
					
					res = true;
				}
			}
		}
    }
    catch ( ... ) 
    {
        std::cout << "GetRoleTopVec:ERR: " << std::endl;
    }
	//printf("MysqlManager::GetRoleTopVec: end \n");
	return res;
}

bool MysqlManager::CheckValidVec(Octets & sqltext)
{
	size_t sqlvec_size = sqltext.size();
	if ( sqlvec_size > 0 && sqlvec_size < 128 )
	{
		std::string sqlvec( (const char*)sqltext.begin() , sqltext.size()-1 );
		for (char c : sqlvec)
		{
			if( !(c >= '0' && c <= '9') &&
				!(c == ',' || c == ':')  )
			return false;
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------//

bool MysqlManager::MakeDefaultPlayer(EC_SQLArenaPlayer & pPlayer, int roleid,  int cls, Octets & name)
{
	pPlayer.player_id = roleid;
	pPlayer.team_id = 0;
	pPlayer.cls = cls;
	pPlayer.score = DEFAULT_ARENA_SCORE;
	pPlayer.win_count = 0;
	pPlayer.team_score = 0;
	pPlayer.week_battle_count = 0;
	pPlayer.invite_time = time(0);
	pPlayer.last_visite = 0;
	pPlayer.battle_count = 0;
	pPlayer.name = name;
	return true;
}

bool MysqlManager::MakeDefaultTeam(EC_SQLArenaTeam & pTeam, int capitan_id, int team_type, Octets & team_name)
{
	pTeam.team_id = 0;
	pTeam.captain_id = capitan_id;
	pTeam.team_type = team_type;
	pTeam.score = DEFAULT_ARENA_SCORE;
	pTeam.last_visite = 0;
	pTeam.win_count = 0;
	pTeam.team_score = DEFAULT_ARENA_SCORE;
	pTeam.week_battle_count = 0;
	pTeam.create_time = time(0);
	pTeam.battle_count = 0;
	pTeam.name = team_name;
	pTeam.members.push_back(EC_SQLArenaTeamMember(capitan_id, 0));
	pTeam.teams.clear();
	return true;
}

//-----------------------------------------------------------------------------//

int MysqlManager::GetArenaPlayer( int roleid, EC_SQLArenaPlayer & pPlayer )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	sprintf((char*)iSQL.istr[i++].resize( LEN__QUERY).begin(), "SELECT * FROM `arena_players` WHERE player_id = '%d'", roleid );
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 && iSQL.ostr[0].str.size() >= 11 )
	{
		pPlayer.player_id = GetRowNum(iSQL, 0, 0);
		pPlayer.team_id = GetRowNum(iSQL, 0, 1);
		pPlayer.cls = GetRowNum(iSQL, 0, 2);
		pPlayer.score = GetRowNum(iSQL, 0, 3);
		pPlayer.win_count = GetRowNum(iSQL, 0, 4);
		pPlayer.team_score = GetRowNum(iSQL, 0, 5);
		pPlayer.week_battle_count = GetRowNum(iSQL, 0, 6);
		pPlayer.invite_time = GetRowNum(iSQL, 0, 7);
		pPlayer.last_visite = GetRowNum(iSQL, 0, 8);
		pPlayer.battle_count = GetRowNum(iSQL, 0, 9);
		GetName(GetRowOct(iSQL, 0, 10), pPlayer.name);

		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::SetArenaPlayer( int roleid, EC_SQLArenaPlayer & pPlayer )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);

	Octets name_base64;
	SetName(name_base64, pPlayer.name);

	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
		"REPLACE INTO arena_players VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%.*s')", 
		pPlayer.player_id,
		pPlayer.team_id,
		pPlayer.cls,
		pPlayer.score,
		pPlayer.win_count,
		pPlayer.team_score,
		pPlayer.week_battle_count,
		pPlayer.invite_time,
		pPlayer.last_visite,
		pPlayer.battle_count,
		name_base64.size(),
		name_base64.begin()
	);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::CreateArenaPlayer(int roleid, int cls, Octets & name, EC_SQLArenaPlayer & pPlayer )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN

	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	MakeDefaultPlayer(pPlayer, roleid, cls, name);

	Octets name_base64;
	SetName(name_base64, pPlayer.name);
	
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
		"REPLACE INTO arena_players VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%.*s')", 
		pPlayer.player_id,
		pPlayer.team_id,
		pPlayer.cls,
		pPlayer.score,
		pPlayer.win_count,
		pPlayer.team_score,
		pPlayer.week_battle_count,
		pPlayer.invite_time,
		pPlayer.last_visite,
		pPlayer.battle_count,
		name_base64.size(),
		name_base64.begin()
	);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::DeleteArenaPlayer(int roleid )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);

	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(), "DELETE FROM `arena_players` WHERE `arena_players`.`player_id` = '%d'", roleid);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}

	MYSQL_MUTEX_END
	return result;
}

//-----------------------------------------------------------------------------//

int MysqlManager::GetArenaTeam( int team_id, int capitan_id, EC_SQLArenaTeam & pTeam )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN

	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	sprintf((char*)iSQL.istr[i++].resize( LEN__QUERY).begin(), "SELECT * FROM `arena_teams` WHERE team_id = '%d' or captain_id = '%d'", team_id , capitan_id );

	if ( MysqlSender(iSQL) && iSQL.ostr.size() >= 1 && iSQL.ostr[0].str.size() >= 13 )
	{
		pTeam.team_id = GetRowNum(iSQL, 0, 0);
		pTeam.captain_id = GetRowNum(iSQL, 0, 1);
		pTeam.team_type = GetRowNum(iSQL, 0, 2);
		pTeam.score = GetRowNum(iSQL, 0, 3);
		pTeam.last_visite = GetRowNum(iSQL, 0, 4);
		pTeam.win_count = GetRowNum(iSQL, 0, 5);
		pTeam.team_score = GetRowNum(iSQL, 0, 6);
		pTeam.week_battle_count = GetRowNum(iSQL, 0, 7);
		pTeam.create_time = GetRowNum(iSQL, 0, 8);
		pTeam.battle_count = GetRowNum(iSQL, 0, 9);
		GetName(GetRowOct(iSQL, 0, 10), pTeam.name);
		GetRoleVec(GetRowOct(iSQL, 0, 11), pTeam.members);
		GetIntVec(GetRowOct(iSQL, 0, 12), pTeam.teams);
		
		result = ERR_SUCCESS;
	}
	
	//printf("EC_GetArenaTeam_Re: team=%d , player=%d , member=%d , size=%d \n", pTeam.team_id, capitan_id, pTeam.members[0].player_id, pTeam.members.size());
	
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::SetArenaTeam( int team_id, int capitan_id, EC_SQLArenaTeam & pTeam )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	Octets name_base64;
	SetName(name_base64, pTeam.name);
	
	std::string members;
	SetRoleVec(members, pTeam.members);
	
	std::string teams;
	SetIntVec(teams, pTeam.teams);
	
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(),
		"REPLACE INTO arena_teams VALUES ( '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%.*s', '%s', '%s' )", 
		pTeam.team_id,
		pTeam.captain_id,
		pTeam.team_type,
		pTeam.score,
		pTeam.last_visite,
		pTeam.win_count,
		pTeam.team_score,
		pTeam.week_battle_count,
		pTeam.create_time,
		pTeam.battle_count,
		name_base64.size(),
		name_base64.begin(),
		members.c_str(),
		teams.c_str()
	);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::CreateArenaTeam( int capitan_id, int team_type, Octets & team_name, EC_SQLArenaTeam & pTeam )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);

	MakeDefaultTeam(pTeam, capitan_id, team_type, team_name);
	
	Octets name_base64;
	SetName(name_base64, pTeam.name);	
	
	std::string members;
	SetRoleVec(members, pTeam.members);
	
	std::string teams;
	SetIntVec(teams, pTeam.teams);
	
	sprintf((char*)iSQL.istr[i++].resize( LEN__QUERY).begin(), 
		"INSERT INTO `arena_teams` ( `captain_id`, `team_type`, `score`, `last_visite`, `win_count`, `team_score`, `week_battle_count`, `create_time`, `battle_count`, `name`, `members`, `teams`) VALUES ( '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%.*s', '%s', '%s' )", 
		pTeam.captain_id,
		pTeam.team_type,
		pTeam.score,
		pTeam.last_visite,
		pTeam.win_count,
		pTeam.team_score,
		pTeam.week_battle_count,
		pTeam.create_time,
		pTeam.battle_count,
		name_base64.size(),
		name_base64.begin(),
		members.c_str(),
		teams.c_str()
	);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::DeleteArenaTeam( int team_id )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);

	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(), "DELETE FROM `arena_teams` WHERE `arena_teams`.`team_id` = '%d'", team_id);
	
	if ( MysqlSender(iSQL) )
	{
		result = ERR_SUCCESS;
	}

	MYSQL_MUTEX_END
	return result;
}

//-----------------------------------------------------------------------------//

int MysqlManager::MakeArenaPlayerTopList( EC_ArenaPlayerTopListVector & vec )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(), "SELECT * FROM `arena_players` ORDER BY `score` DESC LIMIT 200");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() )
	{
		size_t oSize = iSQL.ostr.size();
		
		if (oSize > LEN__OUTPUT)
		{
			oSize = LEN__OUTPUT;
		}
		
		vec.clear();
		
		for (size_t j = 0; j < oSize && j < LEN__OUTPUT; j++ )
		{
			if ( iSQL.ostr[j].str.size() >= 11 )
			{
				//printf("MysqlManager::MakeArenaPlayerTopList:BEGIN: vec_size=%d pos=%d \n", vec.size(), j );
				EC_ArenaPlayerTopList pPlayer;
				pPlayer.player_id  	= GetRowNum(iSQL, j, 0);
				pPlayer.team_id	  	= GetRowNum(iSQL, j, 1);
				pPlayer.cls		  	= GetRowNum(iSQL, j, 2);
				pPlayer.score		= GetRowNum(iSQL, j, 3);
				pPlayer.win_count	= GetRowNum(iSQL, j, 4);
				pPlayer.team_score	= GetRowNum(iSQL, j, 5);
				pPlayer.pos		  	= j;
				GetName(GetRowOct(iSQL, j, 10), pPlayer.name);
				vec.push_back(pPlayer);
				//printf("MysqlManager::MakeArenaPlayerTopList:END: vec_size=%d , roleid=%d \n", vec.size() , pPlayer.player_id );
			}
		}
		
		result = ERR_SUCCESS;
	}
	
	//printf("MysqlManager::MakeArenaPlayerTopList: vec_size=%d \n", vec.size() );
	MYSQL_MUTEX_END
	return result;
}

int MysqlManager::MakeArenaTeamTopList( EC_ArenaTeamTopListVector & vec )
{
	int result = -1;
	MYSQL_MUTEX_BEGIN
	
	size_t i = 0;
	GSQL iSQL(0, 0, 0);
	size_t SqlCount = 1;
	iSQL.istr.resize(SqlCount);
	
	sprintf((char*)iSQL.istr[i++].resize(LEN__QUERY).begin(), "SELECT * FROM `arena_teams` ORDER BY `team_score` DESC LIMIT 50");
	
	if ( MysqlSender(iSQL) && iSQL.ostr.size() )
	{
		size_t oSize = iSQL.ostr.size();
		
		if (oSize > LEN__OUTPUT)
		{
			oSize = LEN__OUTPUT;
		}
		
		vec.clear();
		vec.resize(oSize);
		
		for (size_t j = 0; j < oSize && j < LEN__OUTPUT; j++ )
		{
			if ( iSQL.ostr[j].str.size() >= 13 )
			{
				vec[j].team_id    = GetRowNum(iSQL, j, 0);
				vec[j].captain_id = GetRowNum(iSQL, j, 1);
				vec[j].team_type  = GetRowNum(iSQL, j, 2);
				vec[j].score      = GetRowNum(iSQL, j, 3);
				vec[j].win_count  = GetRowNum(iSQL, j, 5);
				vec[j].team_score = GetRowNum(iSQL, j, 6);
				GetName(GetRowOct(iSQL, j, 10), vec[j].name);
				GetRoleTopVec(GetRowOct(iSQL, j, 11), vec[j].members);
			}
		}
		
		result = ERR_SUCCESS;
	}
	
	MYSQL_MUTEX_END
	return result;
}

//-----------------------------------------------------------------------------//

bool MysqlManager::PanelQuery(GSQL & iSQL)
{
	bool res = false;
	MYSQL_MUTEX_BEGIN
	
	if ( MysqlSender(iSQL) )
	{
		res = true;
	}
	
	MYSQL_MUTEX_END
	return res;
}

//-----------------------------------------------------------------------------//
//-------------------------------[MYSQL_TIMER]---------------------------------//
//-----------------------------------------------------------------------------//

void MysqlTimer::UpdateTimer()
{
	static size_t tick = 0;
	tick++;
	MysqlManager::GetInstance()->HeartBeat(tick);
}

void MysqlTimer::Run()
{
	UpdateTimer();
	Thread::HouseKeeper::AddTimerTask(this,update_time);
}



