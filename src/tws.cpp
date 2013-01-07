/*** tws.cpp -- tws c portion
 *
 * Copyright (C) 2012-2013 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of twsgluum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */

#include <twsapi/EWrapper.h>
#include <twsapi/EPosixClientSocket.h>
#include "tws.h"
#include "nifty.h"

#define TWS_WRP(x)	((__wrapper*)x)
#define TWS_CLI(x)	((IB::EPosixClientSocket*)(TWS_WRP(x))->cli)
#define TWS_PRIV_WRP(x)	TWS_WRP(x)
#define TWS_PRIV_CLI(x)	TWS_CLI(x)

typedef unsigned int tws_oid_t;
typedef unsigned int tws_time_t;

class __wrapper: public IB::EWrapper
{
public:
	void tickPrice(IB::TickerId, IB::TickType, double pri, int autop);
	void tickSize(IB::TickerId, IB::TickType, int size);
	void tickOptionComputation(
		IB::TickerId, IB::TickType,
		double imp_vol, double delta,
		double pri, double div, double gamma, double vega,
		double theta, double undly_pri);
	void tickGeneric(IB::TickerId, IB::TickType, double value);
	void tickString(IB::TickerId, IB::TickType, const IB::IBString&);
	void tickEFP(
		IB::TickerId, IB::TickType,
		double basisPoints, const IB::IBString &formattedBasisPoints,
		double totalDividends, int holdDays,
		const IB::IBString& futureExpiry, double dividendImpact,
		double dividendsToExpiry);
	void orderStatus(
		IB::OrderId, const IB::IBString &status,
		int filled, int remaining, double avgFillPrice, int permId,
		int parentId, double lastFillPrice, int clientId,
		const IB::IBString& whyHeld);
	void openOrder(
		IB::OrderId, const IB::Contract&,
		const IB::Order&, const IB::OrderState&);
	void openOrderEnd(void);
	void winError(const IB::IBString &str, int lastError);
	void connectionClosed(void);
	void updateAccountValue(
		const IB::IBString &key, const IB::IBString &val,
		const IB::IBString &currency, const IB::IBString &accountName);
	void updatePortfolio(
		const IB::Contract&, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL,
		const IB::IBString& accountName);
	void updateAccountTime(const IB::IBString &timeStamp);
	void accountDownloadEnd(const IB::IBString &accountName);
	void nextValidId(IB::OrderId orderId);
	void contractDetails(int req_id, const IB::ContractDetails&);
	void bondContractDetails(int req_id, const IB::ContractDetails&);
	void contractDetailsEnd(int req_id);
	void execDetails(int req_id, const IB::Contract&, const IB::Execution&);
	void execDetailsEnd(int reqId);
	void error(const int id, const int errorCode, const IB::IBString);
	void updateMktDepth(
		IB::TickerId,
		int position, int operation, int side, double price, int size);
	void updateMktDepthL2(
		IB::TickerId,
		int position, IB::IBString marketMaker,
		int operation, int side, double price, int size);
	void updateNewsBulletin(
		int msgId, int msgType,
		const IB::IBString &newsMessage, const IB::IBString &originExch);
	void managedAccounts(const IB::IBString &accountsList);
	void receiveFA(IB::faDataType, const IB::IBString &cxml);
	void historicalData(
		IB::TickerId,
		const IB::IBString &date,
		double open, double high, double low, double close, int volume,
		int barCount, double WAP, int hasGaps);
	void scannerParameters(const IB::IBString &xml);
	void scannerData(
		int reqId, int rank,
		const IB::ContractDetails&,
		const IB::IBString &distance, const IB::IBString &benchmark,
		const IB::IBString &projection, const IB::IBString &legsStr);
	void scannerDataEnd(int reqId);
	void realtimeBar(
		IB::TickerId, long time,
		double open, double high, double low, double close,
		long volume, double wap, int count);
	void currentTime(long time);
	void fundamentalData(IB::TickerId, const IB::IBString &data);
	void deltaNeutralValidation(int reqId, const IB::UnderComp&);
	void tickSnapshotEnd(int reqId);
	void marketDataType(IB::TickerId reqId, int mkt_data_type);

	/* sort of private */
	tws_oid_t next_oid;
	tws_time_t time;
	void *cli;
};

#if defined ASPECT_QUO
# include "quo-wrapper.cpp"
#else
# include "empty-wrapper.cpp"
#endif


static void
rset_tws(tws_t tws)
{
	TWS_PRIV_WRP(tws)->time = 0;
	TWS_PRIV_WRP(tws)->next_oid = 0;
	return;
}

static int
tws_start(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->handshake();
}

static int
tws_stop(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->wavegoodbye();
}

static inline int
__started_p(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->handshake() == 1;
}

static inline int
__sock_ok_p(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->isSocketOK();
}


// public funs
tws_t
init_tws(int sock, int client)
{
	tws_t res = (tws_t)new __wrapper();

	rset_tws(res);
	TWS_PRIV_WRP(res)->cli = new IB::EPosixClientSocket(TWS_PRIV_WRP(res));

	TWS_PRIV_CLI(res)->prepareHandshake(sock, client);
	tws_start(res);
	return res;
}

int
fini_tws(tws_t tws)
{
	if (UNLIKELY(tws == NULL)) {
		/* already finished */
		return 0;
	}
	if (TWS_PRIV_CLI(tws)) {
		/* perform API internal stopping routine */
		tws_stop(tws);

		delete TWS_PRIV_CLI(tws);
		TWS_PRIV_WRP(tws)->cli = NULL;
	}
	if (TWS_PRIV_WRP(tws)) {
		delete TWS_PRIV_WRP(tws);
	}
	return 0;
}

int
tws_send(tws_t tws)
{
	TWS_PRIV_CLI(tws)->onSend();
	return __sock_ok_p(tws) ? 0 : -1;
}

int
tws_recv(tws_t tws)
{
	TWS_PRIV_CLI(tws)->onReceive();
	return __sock_ok_p(tws) ? 0 : -1;
}

tws_st_t
tws_state(tws_t tws)
{
	if (UNLIKELY(tws == NULL || TWS_PRIV_CLI(tws) == NULL)) {
		return TWS_ST_UNK;
	} else if (!__sock_ok_p(tws)) {
		/* fucking great */
		return TWS_ST_DWN;
	} else if (!__started_p(tws)) {
		/* in the middle of a set-up */
		return TWS_ST_SUP;
	} else if (!TWS_PRIV_WRP(tws)->next_oid) {
		/* we get the next_oid automatically */
		return TWS_ST_SUP;
	}
	/* nothing else to assume */
	return TWS_ST_RDY;
}

/* tws.cpp ends here */
