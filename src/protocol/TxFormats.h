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

#ifndef SKYWELL_PROTOCOL_TXFORMATS_H_INCLUDED
#define SKYWELL_PROTOCOL_TXFORMATS_H_INCLUDED

#include <protocol/KnownFormats.h>

namespace skywell {

/** Transaction type identifiers.

    These are part of the binary message format.

    @ingroup protocol
*/
enum TxType
{
    ttINVALID           = -1,

    ttPAYMENT           = 0,
    ttFEE               = 101
    
};

/** Manages the list of known transaction formats.
*/
class TxFormats : public KnownFormats <TxType>
{
private:
    void addCommonFields (Item& item);

public:
    /** Create the object.
        This will load the object will all the known transaction formats.
    */
    TxFormats ();

    static TxFormats const& getInstance ();
};

} // skywell

#endif
