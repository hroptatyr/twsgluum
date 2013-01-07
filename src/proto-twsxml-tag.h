#if !defined INCLUDED_proto_twsxml_tag_h_
#define INCLUDED_proto_twsxml_tag_h_

/* all the tags we know about */
typedef enum {
	/* must be first */
	TX_TAG_UNK,
	/* alphabetic list of tags */
	TX_TAG_COMBOLEGS,
	TX_TAG_COMBOLEG,
	TX_TAG_CONTRACT,
	TX_TAG_CONTRACTDETAILS,
	TX_TAG_ORDER,
	TX_TAG_QUERY,
	TX_TAG_REQCONTRACT,
	TX_TAG_REQUEST,
	TX_TAG_RESPONSE,
	TX_TAG_SUMMARY,
	TX_TAG_TWSXML,
} tws_xml_tid_t;

#endif	/* INCLUDED_proto_twsxml_tag_h_ */
