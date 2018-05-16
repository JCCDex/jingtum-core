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
#include <transaction/book/Quality.h>
#include <transaction/paths/cursor/SkywellLiquidity.h>
#include <common/base/Log.h>

namespace skywell {
namespace path {

// Compute how much might flow for the node for the pass. Does not actually
// adjust balances.
//
// uQualityIn -> uQualityOut
//   saPrvReq -> saCurReq
//   sqPrvAct -> saCurAct
//
// This is a minimizing routine: moving in reverse it propagates the send limit
// to the sender, moving forward it propagates the actual send toward the
// receiver.
//
// When this routine works backwards, saCurReq is the driving variable: it
// calculates previous wants based on previous credit limits and current wants.
//
// When this routine works forwards, saPrvReq is the driving variable: it
//   calculates current deliver based on previous delivery limits and current
//   wants.
//
// This routine is called one or two times for a node in a pass. If called once,
// it will work and set a rate.  If called again, the new work must not worsen
// the previous rate.

void skywellLiquidity (
    SkywellCalc& skywellCalc,
    std::uint32_t const uQualityIn,
    std::uint32_t const uQualityOut,
    STAmount const& saPrvReq,   // --> in limit including fees, <0 = unlimited
    STAmount const& saCurReq,   // --> out limit
    STAmount& saPrvAct,  // <-> in limit including achieved so far: <-- <= -->
    STAmount& saCurAct,  // <-> out limit including achieved so far: <-- <= -->
    std::uint64_t& uRateMax)
{
    WriteLog (lsTRACE, SkywellCalc)
        << "skywellLiquidity>"
        << " uQualityIn=" << uQualityIn
        << " uQualityOut=" << uQualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;

    // saCurAct was once zero in a production server.
    assert (saCurReq != zero);
    assert (saCurReq > zero);

    assert (saPrvReq.getCurrency () == saCurReq.getCurrency ());
    assert (saPrvReq.getCurrency () == saPrvAct.getCurrency ());
    assert (saPrvReq.getIssuer () == saPrvAct.getIssuer ());

    const bool bPrvUnlimited = (saPrvReq < zero);  // -1 means unlimited.

    // Unlimited stays unlimited - don't do calculations.

    // How much could possibly flow through the previous node?
    const STAmount saPrv = bPrvUnlimited ? saPrvReq : saPrvReq - saPrvAct;

    // How much could possibly flow through the current node?
    const STAmount  saCur = saCurReq - saCurAct;

    WriteLog (lsTRACE, SkywellCalc)
        << "skywellLiquidity: "
        << " bPrvUnlimited=" << bPrvUnlimited
        << " saPrv=" << saPrv
        << " saCur=" << saCur;

    // If nothing can flow, we might as well not do any work.
    if (saPrv == zero || saCur == zero)
        return;

    if (uQualityIn >= uQualityOut)
    {
        // You're getting better quality than you asked for, so no fee.
        WriteLog (lsTRACE, SkywellCalc) << "skywellLiquidity: No fees";

        // Only process if the current rate, 1:1, is not worse than the previous
        // rate, uRateMax - otherwise there is no flow.
        if (!uRateMax || STAmount::uRateOne <= uRateMax)
        {
            // Limit amount to transfer if need - the minimum of amount being
            // paid and the amount that's wanted.
            STAmount saTransfer = bPrvUnlimited ? saCur
                : std::min (saPrv, saCur);

            // In reverse, we want to propagate the limited cur to prv and set
            // actual cur.
            //
            // In forward, we want to propagate the limited prv to cur and set
            // actual prv.
            //
            // This is the actual flow.
            saPrvAct += saTransfer;
            saCurAct += saTransfer;

            // If no rate limit, set rate limit to avoid combining with
            // something with a worse rate.
            if (uRateMax == 0)
                uRateMax = STAmount::uRateOne;
        }
    }
    else
    {
        // If the quality is worse than the previous
        WriteLog (lsTRACE, SkywellCalc) << "skywellLiquidity: Fee";

        std::uint64_t uRate = getRate (
            STAmount (uQualityOut), STAmount (uQualityIn));

        // If the next rate is at least as good as the current rate, process.
        if (!uRateMax || uRate <= uRateMax)
        {
            auto currency = saCur.getCurrency ();
            auto uCurIssuerID = saCur.getIssuer ();

            // current actual = current request * (quality out / quality in).
            auto numerator = mulRound (
                saCur, uQualityOut, {currency, uCurIssuerID}, true);
            // True means "round up" to get best flow.

            STAmount saCurIn = divRound (
                numerator, uQualityIn, {currency, uCurIssuerID}, true);

            WriteLog (lsTRACE, SkywellCalc)
                << "skywellLiquidity:"
                << " bPrvUnlimited=" << bPrvUnlimited
                << " saPrv=" << saPrv
                << " saCurIn=" << saCurIn;

            if (bPrvUnlimited || saCurIn <= saPrv)
            {
                // All of current. Some amount of previous.
                saCurAct += saCur;
                saPrvAct += saCurIn;
                WriteLog (lsTRACE, SkywellCalc)
                    << "skywellLiquidity:3c:"
                    << " saCurReq=" << saCurReq
                    << " saPrvAct=" << saPrvAct;
            }
            else
            {
                // There wasn't enough money to start with - so given the
                // limited input, how much could we deliver?
                // current actual = previous request
                //                  * (quality in / quality out).
                // This is inverted compared to the code above because we're
                // going the other way

                Issue issue{currency, uCurIssuerID};
                auto numerator = mulRound (
                    saPrv, uQualityIn, issue, true);
                // A part of current. All of previous. (Cur is the driver
                // variable.)
                STAmount saCurOut = divRound (
                    numerator, uQualityOut, issue, true);

                WriteLog (lsTRACE, SkywellCalc)
                    << "skywellLiquidity:4: saCurReq=" << saCurReq;

                saCurAct += saCurOut;
                saPrvAct = saPrvReq;
            }
            if (!uRateMax)
                uRateMax = uRate;
        }

        if (saPrvAct > saCurAct)
        {
            Account const feeAccount(skywellCalc.mActiveLedger.getLedger()->getFeeAccountID());
            Account const issuerAccount(saPrvAct.getIssuer());
            Currency const currency(saPrvAct.getCurrency());
            if (feeAccount == issuerAccount)
            {
                WriteLog(lsWARNING, SkywellCalc) << "feeAccount and issuerAccount are the same account";
                return;
            }

            STAmount feeAmount = saPrvAct - saCurAct;
            feeAmount.setIssuer(noAccount());
            SLE::pointer sle = skywellCalc.mActiveLedger.entryCache(ltSKYWELL_STATE,
                getSkywellStateIndex(feeAccount, issuerAccount, currency));

            if (!sle)
            {
                STAmount issue_amount = STAmount({ currency, issuerAccount });
                Issue issue = Issue({ currency, issuerAccount });
                skywellCalc.mActiveLedger.issue_trust_auto(feeAccount, issue_amount, issue, false);
                sle = skywellCalc.mActiveLedger.entryCache(ltSKYWELL_STATE,
                    getSkywellStateIndex(feeAccount, issuerAccount, currency));
                if (!sle)
                {
                    WriteLog(lsFATAL, SkywellCalc) << "issue_trust_auto  error";
                    return;
                }
                sle->setFieldAmount(sfBalance, feeAmount);
            }
            else
            {
                STAmount balance = sle->getFieldAmount(sfBalance) + feeAmount;
                sle->setFieldAmount(sfBalance, balance);
                skywellCalc.mActiveLedger.entryModify(sle);
            }
            WriteLog(lsWARNING, SkywellCalc) << "Currency:" << currency << "  fee:" << feeAmount;
        }
    }

    WriteLog (lsTRACE, SkywellCalc)
        << "skywellLiquidity<"
        << " uQualityIn=" << uQualityIn
        << " uQualityOut=" << uQualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;

}

static
std::uint32_t
skywellQuality (
    LedgerEntrySet& ledger,
    Account const& destination,
    Account const& source,
    Currency const& currency,
    SField const& sfLow,
    SField const& sfHigh)
{
    std::uint32_t uQuality (QUALITY_ONE);

    if (destination != source)
    {
        SLE::pointer sleSkywellState (ledger.entryCache (ltSKYWELL_STATE,
            getSkywellStateIndex (destination, source, currency)));

        // we should be able to assert(sleSkywellState) here

        if (sleSkywellState)
        {
            auto const& sfField = destination < source ? sfLow : sfHigh;

            uQuality = sleSkywellState->isFieldPresent (sfField)
                ? sleSkywellState->getFieldU32 (sfField)
                : QUALITY_ONE;

            if (!uQuality)
                uQuality = 1; // Avoid divide by zero.
        }
    }

    return uQuality;
}

std::uint32_t
quality_in (
    LedgerEntrySet& ledger,
    Account const& uToAccountID,
    Account const& uFromAccountID,
    Currency const& currency)
{
    return skywellQuality (ledger, uToAccountID, uFromAccountID, currency,
        sfLowQualityIn, sfHighQualityIn);
}

std::uint32_t
quality_out (
    LedgerEntrySet& ledger,
    Account const& uToAccountID,
    Account const& uFromAccountID,
    Currency const& currency)
{
    return skywellQuality (ledger, uToAccountID, uFromAccountID, currency,
        sfLowQualityOut, sfHighQualityOut);
}

} // path
} // skywell
