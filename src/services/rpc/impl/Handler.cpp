//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012, 2013 Skywell Labs Inc.

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
#include <services/rpc/impl/Handler.h>
#include <services/rpc/handlers/Handlers.h>
#include <services/rpc/handlers/Ledger.h>
#include <services/rpc/handlers/Version.h>

namespace skywell {
namespace RPC {
namespace {

/** Adjust an old-style handler to be call-by-reference. */
template <typename Function>
Handler::Method<Json::Value> byRef (Function const& f)
{
    return [f] (Context& context, Json::Value& result)
    {
        result = f (context);
        if (result.type() != Json::objectValue)
        {
            assert (false);
            result = RPC::makeObjectValue (result);
        }

        return Status();
    };
}

template <class Object, class HandlerImpl>
Status handle (Context& context, Object& object)
{
    HandlerImpl handler (context);

    auto status = handler.check ();
    if (status)
        status.inject (object);
    else
        handler.writeResult (object);
    return status;
};

class HandlerTable {
  public:
    HandlerTable (std::vector<Handler> const& entries) {
        for (auto& entry: entries)
        {
            assert (table_.find(entry.name_) == table_.end());
            table_[entry.name_] = entry;
        }

        // This is where the new-style handlers are added.
        addHandler<LedgerHandler>();
        addHandler<VersionHandler>();
    }

    const Handler* getHandler(std::string name) {
        auto i = table_.find(name);
        return i == table_.end() ? nullptr : &i->second;
    }

  private:
    std::map<std::string, Handler> table_;

    template <class HandlerImpl>
    void addHandler()
    {
        assert (table_.find(HandlerImpl::name()) == table_.end());

        Handler h;
        h.name_ = HandlerImpl::name();
        h.valueMethod_ = &handle<Json::Value, HandlerImpl>;
        h.role_ = HandlerImpl::role();
        h.condition_ = HandlerImpl::condition();
        h.objectMethod_ = &handle<Json::Object, HandlerImpl>;

        table_[HandlerImpl::name()] = h;
    };
};

HandlerTable HANDLERS({
    // Some handlers not specified here are added to the table via addHandler()
    // Request-response methods
    {   "account_info",         byRef (&doAccountInfo),         Role::USER,  NO_CONDITION  },
    {   "account_tx",           byRef (&doAccountTxSwitch),     Role::USER,  NO_CONDITION  },
    {   "ledger_accept",        byRef (&doLedgerAccept),        Role::ADMIN,   NEEDS_CURRENT_LEDGER  },
    {   "ledger_cleaner",       byRef (&doLedgerCleaner),       Role::ADMIN,   NEEDS_NETWORK_CONNECTION  },
    {   "ledger_closed",        byRef (&doLedgerClosed),        Role::USER,  NO_CONDITION   },
    {   "ledger_current",       byRef (&doLedgerCurrent),       Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "submit",               byRef (&doSubmit),              Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "server_info",          byRef (&doServerInfo),          Role::USER,  NO_CONDITION     },
    {   "server_state",         byRef (&doServerState),         Role::USER,  NO_CONDITION     },
    {   "stop",                 byRef (&doStop),                Role::ADMIN,   NO_CONDITION     },
    {   "validation_create",    byRef (&doValidationCreate),    Role::ADMIN,   NO_CONDITION     },
    {   "wallet_propose",       byRef (&doWalletPropose),       Role::ADMIN,   NO_CONDITION     },
	 
    // Evented methods
});

} // namespace

const Handler* getHandler(std::string const& name) {
    return HANDLERS.getHandler(name);
}

} // RPC
} // skywell
