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

void
__wrapper::tickPrice(IB::TickerId id, IB::TickType fld, double pri, int)
{
	return;
}

void
__wrapper::tickSize(IB::TickerId id, IB::TickType fld, int size)
{
	return;
}

void
__wrapper::tickOptionComputation(
	IB::TickerId id, IB::TickType fld,
	double, double,
	double, double, double, double,
	double, double)
{
	return;
}

void
__wrapper::tickGeneric(IB::TickerId id, IB::TickType fld, double value)
{
	return;
}

void
__wrapper::tickString(IB::TickerId id, IB::TickType fld, const IB::IBString &s)
{
	return;
}

void
__wrapper::tickEFP(
	IB::TickerId id, IB::TickType fld,
	double, const IB::IBString&,
	double, int,
	const IB::IBString&, double, double)
{
	return;
}

void
__wrapper::tickSnapshotEnd(int id)
{
	return;
}

void
__wrapper::marketDataType(IB::TickerId id, int mkt_data_type)
{
	return;
}

void
__wrapper::contractDetails(int req_id, const IB::ContractDetails &cd)
{
	return;
}

void
__wrapper::bondContractDetails(int req_id, const IB::ContractDetails &cd)
{
	return;
}

void
__wrapper::contractDetailsEnd(int req_id)
{
	return;
}

void
__wrapper::fundamentalData(IB::TickerId id, const IB::IBString &data)
{
	return;
}

void
__wrapper::updateMktDepth(
	IB::TickerId id,
	int, int, int, double, int)
{
	return;
}

void
__wrapper::updateMktDepthL2(
	IB::TickerId id,
	int, IB::IBString,
	int, int, double, int)
{
	return;
}

void
__wrapper::historicalData(
	IB::TickerId id, const IB::IBString&,
	double, double, double, double, int,
	int, double, int)
{
	return;
}

void
__wrapper::realtimeBar(
	IB::TickerId id, long,
	double, double, double, double,
	long, double, int)
{
	return;
}


/* trades */
void
__wrapper::orderStatus(
	IB::OrderId id, const IB::IBString &state,
	int flld, int remn, double,
	int perm_id, int parent_id, double prc, int cli,
	const IB::IBString &yheld)
{
	return;
}

void
__wrapper::openOrder(
	IB::OrderId id, const IB::Contract &c,
	const IB::Order &o, const IB::OrderState &s)
{
	return;
}

void
__wrapper::openOrderEnd(void)
{
	return;
}

void
__wrapper::execDetails(int rid, const IB::Contract &c, const IB::Execution &ex)
{
	return;
}

void
__wrapper::execDetailsEnd(int req_id)
{
	return;
}


/* post trade */
void
__wrapper::updateAccountValue(
	const IB::IBString &key, const IB::IBString &val,
	const IB::IBString &ccy, const IB::IBString &acn)
{
	return;
}

void
__wrapper::updatePortfolio(
	const IB::Contract &c, int pos,
	double, double mkt_val, double, double, double,
	const IB::IBString &acn)
{
	return;
}

void
__wrapper::updateAccountTime(const IB::IBString&)
{
	return;
}

void
__wrapper::accountDownloadEnd(const IB::IBString &name)
{
	return;
}

void
__wrapper::managedAccounts(const IB::IBString &ac)
{
	return;
}


/* infra */
void
__wrapper::error(const int id, const int code, const IB::IBString msg)
{
	return;
}

void
__wrapper::winError(const IB::IBString &str, int code)
{
	return;
}

void
__wrapper::connectionClosed(void)
{
	return;
}


/* stuff that doesn't do calling-back at all */
void
__wrapper::currentTime(long int time)
{
/* not public */
	this->time = time;
	return;
}

void
__wrapper::nextValidId(IB::OrderId oid)
{
/* not public */
	this->next_oid = oid;
	return;
}

void
__wrapper::scannerParameters(const IB::IBString&)
{
	return;
}

void
__wrapper::scannerData(
	int, int,
	const IB::ContractDetails&,
	const IB::IBString&, const IB::IBString&,
	const IB::IBString&, const IB::IBString&)
{
	return;
}

void
__wrapper::scannerDataEnd(int)
{
	return;
}

void
__wrapper::deltaNeutralValidation(int, const IB::UnderComp&)
{
	return;
}

void
__wrapper::updateNewsBulletin(
	int, int,
	const IB::IBString&, const IB::IBString&)
{
	return;
}

void
__wrapper::receiveFA(IB::faDataType, const IB::IBString&)
{
	return;
}


static void
rset_tws(tws_t tws)
{
	TWS_PRIV_WRP(tws)->time = 0;
	TWS_PRIV_WRP(tws)->next_oid = 0;
	return;
}

int
tws_started_p(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->handshake() == 1;
}

static int
tws_start(tws_t tws)
{
	int st;

	if ((st = TWS_PRIV_CLI(tws)->handshake()) == 1) {
		TWS_PRIV_CLI(tws)->reqCurrentTime();
	}
	return st;
}

static int
tws_stop(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->wavegoodbye();
}

static inline int
__sock_ok_p(tws_t tws)
{
	return TWS_PRIV_CLI(tws)->isSocketOK() ? 0 : -1;
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
	/* we used to call tws_disconnect() here but that's ancient history
	 * just like we don't call tws_connect() in tws_init() we won't call
	 * tws_disconnect() here. */
	tws_stop(tws);
	/* wipe our context off the face of this earth */
	rset_tws(tws);

	if (TWS_PRIV_CLI(tws)) {
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
	return __sock_ok_p(tws);
}

int
tws_recv(tws_t tws)
{
	TWS_PRIV_CLI(tws)->onReceive();
	return __sock_ok_p(tws);
}

/* tws.cpp ends here */
