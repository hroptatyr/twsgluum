/*** empty-wrapper.cpp -- just a husk
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
#if !defined INCLUDED_empty_wrapper_cpp_ && !defined ASPECT_WRAPPER_
#define INCLUDED_empty_wrapper_cpp_
#define ASPECT_WRAPPER_

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

#endif	// INCLUDED_empty_wrapper_cpp_ || ASPECT_WRAPPER_
