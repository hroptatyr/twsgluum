#if !defined INCLUDED_proto_twsxml_reqtyp_h_
#define INCLUDED_proto_twsxml_reqtyp_h_

typedef enum {
	/* must be first */
	TWS_XML_REQ_TYP_UNK,
	/* alphabetic list of tags */
	TWS_XML_REQ_TYP_CON_DTLS,
	TWS_XML_REQ_TYP_HIST_DATA,
	TWS_XML_REQ_TYP_MKT_DATA,
	TWS_XML_REQ_TYP_PLC_ORDER,

} tws_xml_req_typ_t;

#endif	/* INCLUDED_proto_twsxml_reqtyp_h_ */
