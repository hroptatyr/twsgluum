/*** tws.h -- tws c portion
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
#if !defined INCLUDED_tws_h_
#define INCLUDED_tws_h_

#include <stdbool.h>
#include "sdef.h"

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef struct tws_s *tws_t;

typedef enum {
	/** no state can be determined */
	TWS_ST_UNK,
	/** in the state of setting up the connection */
	TWS_ST_SUP,
	/** ready state, you should be able to read and write */
	TWS_ST_RDY,
	/** down state, either finish the conn or re-set it up */
	TWS_ST_DWN,
} tws_st_t;

typedef enum {
	TWS_CB_UNK,

	/* PREs */
	/* .val types */
	TWS_CB_PRE_PRICE,
	TWS_CB_PRE_SIZE,
	TWS_CB_PRE_GENERIC,
	TWS_CB_PRE_SNAP_END,
	/* .i types */
	TWS_CB_PRE_MKT_DATA_TYPE,
	/* .str types */
	TWS_CB_PRE_STRING,
	TWS_CB_PRE_FUND_DATA,
	/* .data types */
	TWS_CB_PRE_CONT_DTL,
	TWS_CB_PRE_CONT_DTL_END,
	TWS_CB_PRE_OPTION,
	TWS_CB_PRE_EFP,
	TWS_CB_PRE_UPD_MKT_DEPTH,
	TWS_CB_PRE_HIST_DATA,
	TWS_CB_PRE_REALTIME_BAR,

	/* TRDs */
	TWS_CB_TRD_ORD_STATUS,
	TWS_CB_TRD_OPEN_ORD,
	TWS_CB_TRD_OPEN_ORD_END,
	TWS_CB_TRD_EXEC_DTL,
	TWS_CB_TRD_EXEC_DTL_END,

	/* POSTs */
	TWS_CB_POST_MNGD_AC,
	TWS_CB_POST_ACUP,
	TWS_CB_POST_ACUP_END,

	/* INFRAs */
	TWS_CB_INFRA_ERROR,
	TWS_CB_INFRA_CONN_CLOSED,
	TWS_CB_INFRA_READY,
} tws_cb_t;

/* tick types */
#define TWS_TICK_TYPE_BID	(1)
#define TWS_TICK_TYPE_BSZ	(0)
#define TWS_TICK_TYPE_ASK	(2)
#define TWS_TICK_TYPE_ASZ	(3)
#define TWS_TICK_TYPE_TRA	(4)
#define TWS_TICK_TYPE_TSZ	(5)
#define TWS_TICK_TYPE_HIGH	(6)
#define TWS_TICK_TYPE_LOW	(7)
#define TWS_TICK_TYPE_VOL	(8)
/* close price at the end of day */
#define TWS_TICK_TYPE_CLOSE	(9)
/* opening price at the beginning of the day */
#define TWS_TICK_TYPE_OPEN	(14)

typedef unsigned int tws_oid_t;
typedef unsigned int tws_time_t;
typedef unsigned int tws_tick_type_t;

/* we split the callbacks into 4 big groups, just like fix:
 * pre_trade, trade, post_trade, infra */
struct tws_pre_clo_s {
	tws_oid_t oid;
	tws_tick_type_t tt;
	union {
		double val;
		const char *str;
		const void *data;
		int i;
	};
};

struct tws_trd_clo_s {
	tws_oid_t oid;
	const void *data;
};

struct tws_post_clo_s {
	tws_oid_t oid;
	const void *data;
};

struct tws_infra_clo_s {
	tws_oid_t oid;
	tws_oid_t code;
	const void *data;
};

struct tws_s {
	void *priv;
	void(*pre_cb)(tws_t, tws_cb_t, struct tws_pre_clo_s);
	void(*trd_cb)(tws_t, tws_cb_t, struct tws_trd_clo_s);
	void(*post_cb)(tws_t, tws_cb_t, struct tws_post_clo_s);
	void(*infra_cb)(tws_t, tws_cb_t, struct tws_infra_clo_s);

	/* flexible array at the end, so users can extend this */
	char user[0];
};


/* structs specific to some callbacks data field */
struct tws_post_acup_clo_s {
	const char *ac_name;
	const void *cont;
	double pos;
	double val;
};

struct tws_post_acup_end_clo_s {
	const char *ac_name;
};

/* used in TRD_ORD_STATUS and TRD_EXEC_DTL */
typedef enum {
	EXEC_TYP_UNK,
	EXEC_TYP_NEW = '0',
	ORD_ST_PFILL = '1',
	ORD_ST_FILL = '2',
	EXEC_TYP_DONE_FOR_DAY = '3',
	EXEC_TYP_CANCELLED = '4',
	EXEC_TYP_REPLACED = '5',
	EXEC_TYP_PENDING_CNC = '6',
	EXEC_TYP_STOPPED = '7',
	EXEC_TYP_REJECTED = '8',
	EXEC_TYP_SUSPENDED = '9',
	EXEC_TYP_PENDING_NEW = 'A',
	EXEC_TYP_CALCULATED = 'B',
	EXEC_TYP_EXPIRED = 'C',
	EXEC_TYP_RESTATED = 'D',
	EXEC_TYP_PENDING_RPLC = 'E',
	EXEC_TYP_TRADE = 'F',
	EXEC_TYP_TRADE_CORRECT = 'G',
	EXEC_TYP_TRADE_CANCEL = 'H',
	EXEC_TYP_ORDER_STATUS = 'I',
	EXEC_TYP_IN_CLEARING_HOLD = 'J',
} fix_st_t;

struct fix_exec_rpt_s {
	fix_st_t exec_typ;
	fix_st_t ord_status;

	double last_qty;
	double last_prc;
	double cum_qty;
	double leaves_qty;

	/** IB's order id */
	tws_oid_t ord_id;
	/** IB's perm id */
	tws_oid_t exec_id;
	/** IB's parent id */
	tws_oid_t exec_ref_id;
	/** IB's client id? */
	tws_oid_t party_id;
};

struct tws_trd_ord_status_clo_s {
	struct fix_exec_rpt_s er;

	const char *yheld;
};

struct tws_trd_exec_dtl_clo_s {
	struct fix_exec_rpt_s er;

	const void *cont;
	const char *exch;
	const char *ac_name;
	const char *ex_time;
};

struct tws_trd_open_ord_clo_s {
	const void *cont;
	const void *order;

	struct {
		fix_st_t state;

		const char *ini_mrgn;
		const char *mnt_mrgn;
		const char *eqty_w_loan;

		double commission;
		double min_comm;
		double max_comm;
		const char *comm_ccy;

		const char *warn;
	} st;
};

struct tws_order_s {
	const char *sym;
	const char *xch;
	const char *typ;
	const char *ccy;

	double amt;
	double lmt;
};


/* connection guts */
extern int init_tws(tws_t, int sock, int client);
extern int fini_tws(tws_t);

extern int tws_recv(tws_t);
extern int tws_send(tws_t);

extern tws_st_t tws_state(tws_t);

/**
 * Request security reference data of the contract C. */
extern tws_oid_t tws_req_sdef(tws_t, const void *c);

/**
 * Subscribe to quote update. */
extern tws_oid_t tws_sub_quo(tws_t, tws_const_sdef_t c);

/**
 * Subscribe to quote update (specified by contract). */
extern tws_oid_t tws_sub_quo_cont(tws_t, tws_const_cont_t c);

/**
 * Unsubscribe from quote update. */
extern int tws_rem_quo(tws_t, tws_oid_t);

/**
 * Subscribe to portfolio and account events. */
extern int tws_sub_ac(tws_t, const char*);

/**
 * Unsubscribe from portfolio and account events. */
extern int tws_rem_ac(tws_t, const char*);


/**
 * Order stuff. */
extern tws_oid_t tws_order(tws_t, struct tws_order_s);

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_tws_h_ */
