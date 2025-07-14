
#ifndef __GNET_MYSQLSTORAGE_RE_HPP
#define __GNET_MYSQLSTORAGE_RE_HPP

#include "rpcdefs.h"
#include "callid.hxx"
#include "state.hxx"

#include "gmysqlrow"

namespace GNET
{

class MySQLStorage_Re : public GNET::Protocol
{
	#include "mysqlstorage_re"

	void Process(Manager *manager, Manager::Session::ID sid)
	{
		// TODO
	}
};

};

#endif
