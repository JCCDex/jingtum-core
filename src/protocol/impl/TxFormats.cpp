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
#include <common/misc/static_initializer.h>
#include <protocol/TxFormats.h>

namespace skywell {

TxFormats::TxFormats ()
{

    add ("Payment", ttPAYMENT)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfSendMax,             SOE_OPTIONAL)
        << SOElement (sfPaths,               SOE_DEFAULT)
        << SOElement (sfInvoiceID,           SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        ;


    add ("SetFee", ttFEE)
        << SOElement (sfLedgerSequence,      SOE_OPTIONAL)
        << SOElement (sfBaseFee,             SOE_REQUIRED)
        << SOElement (sfReferenceFeeUnits,   SOE_REQUIRED)
        << SOElement (sfReserveBase,         SOE_REQUIRED)
        << SOElement (sfReserveIncrement,    SOE_REQUIRED)
        ;


}

void TxFormats::addCommonFields (Item& item)
{
    item
        << SOElement(sfTransactionType,     SOE_REQUIRED)
        << SOElement(sfFlags,               SOE_OPTIONAL)
        << SOElement(sfTimestamp,           SOE_OPTIONAL)
        << SOElement(sfSourceTag,           SOE_OPTIONAL)
        << SOElement(sfAccount,             SOE_REQUIRED)
        << SOElement(sfSequence,            SOE_OPTIONAL/*SOE_REQUIRED*/)
        << SOElement(sfPreviousTxnID,       SOE_OPTIONAL) // emulate027
        << SOElement(sfLastLedgerSequence,  SOE_OPTIONAL)
        << SOElement(sfAccountTxnID,        SOE_OPTIONAL)
        << SOElement(sfFee,                 SOE_OPTIONAL/*SOE_REQUIRED*/)
        << SOElement(sfOperationLimit,      SOE_OPTIONAL)
        << SOElement(sfMemos,               SOE_OPTIONAL)
        << SOElement(sfSigningPubKey,       SOE_OPTIONAL/*SOE_REQUIRED*/)
        << SOElement(sfTxnSignature,        SOE_OPTIONAL)
        ;
}

TxFormats const&
TxFormats::getInstance ()
{
    static skywell::static_initializer<
        TxFormats> instance;
    return *instance;
}

} // skywell
