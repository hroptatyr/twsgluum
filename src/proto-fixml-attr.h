#if !defined INCLUDED_proto_fixml_attr_h_
#define INCLUDED_proto_fixml_attr_h_

typedef enum {
	/* must be first */
	FIX_ATTR_UNK,
	/* alphabetic list of tags */
	FIX_ATTR_CCY,
	FIX_ATTR_CFI,
	FIX_ATTR_EXCH,
	FIX_ATTR_MULT,
	FIX_ATTR_PROD,
	FIX_ATTR_R,
	FIX_ATTR_S,
	FIX_ATTR_SECTYP,
	FIX_ATTR_SYM,
	FIX_ATTR_UOM,
	FIX_ATTR_UOMQTY,
	FIX_ATTR_V,
	FIX_ATTR_XMLNS,

} fixml_aid_t;

#endif	/* INCLUDED_proto_fixml_attr_h_ */
