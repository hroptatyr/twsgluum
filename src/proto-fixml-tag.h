#if !defined INCLUDED_proto_fixml_tag_h_
#define INCLUDED_proto_fixml_tag_h_

/* all the tags we know about */
typedef enum {
	/* must be first */
	FIX_TAG_UNK,
	/* alphabetic list of tags */
	FIX_TAG_BATCH,
	FIX_TAG_FIXML,
	FIX_TAG_INSTRMT,
	FIX_TAG_SECDEF,
} fixml_tid_t;

#endif	/* INCLUDED_proto_fixml_tag_h_ */
