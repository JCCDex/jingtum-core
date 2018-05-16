//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012-2014 Skywell Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <BeastConfig.h>
#include <services/rpc/handlers/ServerInfo.h>
#include <common/core/LoadMonitor.h>
#include <common/misc/NetworkOPs.h>
#include <common/base/StringUtilities.h>
#include <common/json/json_reader.h>
#include <common/core/LoadFeeTrack.h>
#include <protocol/TxFlags.h>
#include <protocol/Indexes.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <ledger/LedgerMaster.h>
#include <services/rpc/impl/KeypairForSignature.h>
#include <services/rpc/impl/TransactionSign.h>
#include <services/rpc/impl/Tuning.h>
#include <services/rpc/impl/LegacyPathFind.h>
#include <services/net/RPCErr.h>
#include <transaction/paths/FindPaths.h>
#include <main/Application.h>
#include <protocol/UintTypes.h>

namespace skywell {


Json::Value doRPCInfo  (RPC::Context& context)
{
    auto& params = context.params;
    Json::Value ret (Json::objectValue);
    bool isReset = params.isMember(jss::reset);
    bool isPrint = params.isMember(jss::print);
    bool isCmd = params.isMember(jss::rpc_command);
    if (isPrint) {
       if (isCmd && !isReset)  {
          RPC::RPCInfo::isprint = true ;
          RPC::RPCInfo::cmdstr = params[jss::rpc_command].asString();
       }
       else if (!isCmd && isReset) {
          RPC::RPCInfo::isprint = false;
       }
       else if (isCmd && isReset )  {
         RPC::RPCInfo::cmdstr.clear();
       }
       else if ( !isCmd && !isReset )  {
         RPC::RPCInfo::cmdstr.clear();
         RPC::RPCInfo::isprint = true ;
       }
    }
    else {
      if (isReset) {
        RPC::RPCInfo::isprint = false;
        RPC::RPCInfo::cmdstr.clear();
        RPC::RPCInfo::reset();
        return ret;
      }
    }
    ret[jss::info] =  RPC::RPCInfo::show(); 
    return ret;
}


Json::Value doServerInfo (RPC::Context& context)
{
    Json::Value ret (Json::objectValue);

    ret[jss::info] = context.netOps.getServerInfo (
        true, context.role == Role::ADMIN);

    return ret;
}


} // skywell
