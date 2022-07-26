/* -*- buffer-read-only: t -*-
 *
 *      Copyright (c) 1996-1999 Malcolm Beattie
 *
 *      You may distribute under the terms of either the GNU General Public
 *      License or the Artistic License, as specified in the README file.
 *
 */
/*
 * This file is autogenerated from bytecode.pl. Changes made here will be lost.
 */
struct byteloader_fdata {
    SV	*datasv;
    int next_out;
    int	idx;
};

struct byteloader_state {
    struct byteloader_fdata	*bs_fdata;
    SV				*bs_sv;
    void			**bs_obj_list;
    int				bs_obj_list_fill;
    int				bs_ix;
    XPV				bs_pv;
    int				bs_iv_overflows;
};

int bl_getc(struct byteloader_fdata *);
int bl_read(struct byteloader_fdata *, char *, size_t, size_t);
extern int byterun(pTHX_ struct byteloader_state *);

enum {
    INSN_RET,			/* 0 */
    INSN_LDSV,			/* 1 */
    INSN_LDOP,			/* 2 */
    INSN_STSV,			/* 3 */
    INSN_STOP,			/* 4 */
    INSN_STPV,			/* 5 */
    INSN_LDSPECSV,			/* 6 */
    INSN_LDSPECSVX,			/* 7 */
    INSN_NEWSV,			/* 8 */
    INSN_NEWSVX,			/* 9 */
    INSN_NOP,			/* 10 */
    INSN_NEWOP,			/* 11 */
    INSN_NEWOPX,			/* 12 */
    INSN_NEWOPN,			/* 13 */
    INSN_NEWPV,			/* 14 */
    INSN_PV_CUR,			/* 15 */
    INSN_PV_FREE,			/* 16 */
    INSN_SV_UPGRADE,			/* 17 */
    INSN_SV_REFCNT,			/* 18 */
    INSN_SV_REFCNT_ADD,			/* 19 */
    INSN_SV_FLAGS,			/* 20 */
    INSN_XRV,			/* 21 */
    INSN_XPV,			/* 22 */
    INSN_XPV_CUR,			/* 23 */
    INSN_XPV_LEN,			/* 24 */
    INSN_XIV,			/* 25 */
    INSN_XNV,			/* 26 */
    INSN_XLV_TARGOFF,			/* 27 */
    INSN_XLV_TARGLEN,			/* 28 */
    INSN_XLV_TARG,			/* 29 */
    INSN_XLV_TYPE,			/* 30 */
    INSN_XBM_USEFUL,			/* 31 */
    INSN_XBM_PREVIOUS,			/* 32 */
    INSN_XBM_RARE,			/* 33 */
    INSN_XFM_LINES,			/* 34 */
    INSN_COMMENT,			/* 35 */
    INSN_XIO_LINES,			/* 36 */
    INSN_XIO_PAGE,			/* 37 */
    INSN_XIO_PAGE_LEN,			/* 38 */
    INSN_XIO_LINES_LEFT,			/* 39 */
    INSN_XIO_TOP_NAME,			/* 40 */
    INSN_XIO_TOP_GV,			/* 41 */
    INSN_XIO_FMT_NAME,			/* 42 */
    INSN_XIO_FMT_GV,			/* 43 */
    INSN_XIO_BOTTOM_NAME,			/* 44 */
    INSN_XIO_BOTTOM_GV,			/* 45 */
    INSN_XIO_SUBPROCESS,			/* 46 */
    INSN_XIO_TYPE,			/* 47 */
    INSN_XIO_FLAGS,			/* 48 */
    INSN_XCV_XSUBANY,			/* 49 */
    INSN_XCV_STASH,			/* 50 */
    INSN_XCV_START,			/* 51 */
    INSN_XCV_ROOT,			/* 52 */
    INSN_XCV_GV,			/* 53 */
    INSN_XCV_FILE,			/* 54 */
    INSN_XCV_DEPTH,			/* 55 */
    INSN_XCV_PADLIST,			/* 56 */
    INSN_XCV_OUTSIDE,			/* 57 */
    INSN_XCV_OUTSIDE_SEQ,			/* 58 */
    INSN_XCV_FLAGS,			/* 59 */
    INSN_AV_EXTEND,			/* 60 */
    INSN_AV_PUSHX,			/* 61 */
    INSN_AV_PUSH,			/* 62 */
    INSN_XAV_FILL,			/* 63 */
    INSN_XAV_MAX,			/* 64 */
    INSN_XAV_FLAGS,			/* 65 */
    INSN_XHV_RITER,			/* 66 */
    INSN_XHV_NAME,			/* 67 */
    INSN_XHV_PMROOT,			/* 68 */
    INSN_HV_STORE,			/* 69 */
    INSN_SV_MAGIC,			/* 70 */
    INSN_MG_OBJ,			/* 71 */
    INSN_MG_PRIVATE,			/* 72 */
    INSN_MG_FLAGS,			/* 73 */
    INSN_MG_NAME,			/* 74 */
    INSN_MG_NAMEX,			/* 75 */
    INSN_XMG_STASH,			/* 76 */
    INSN_GV_FETCHPV,			/* 77 */
    INSN_GV_FETCHPVX,			/* 78 */
    INSN_GV_STASHPV,			/* 79 */
    INSN_GV_STASHPVX,			/* 80 */
    INSN_GP_SV,			/* 81 */
    INSN_GP_REFCNT,			/* 82 */
    INSN_GP_REFCNT_ADD,			/* 83 */
    INSN_GP_AV,			/* 84 */
    INSN_GP_HV,			/* 85 */
    INSN_GP_CV,			/* 86 */
    INSN_GP_FILE,			/* 87 */
    INSN_GP_IO,			/* 88 */
    INSN_GP_FORM,			/* 89 */
    INSN_GP_CVGEN,			/* 90 */
    INSN_GP_LINE,			/* 91 */
    INSN_GP_SHARE,			/* 92 */
    INSN_XGV_FLAGS,			/* 93 */
    INSN_OP_NEXT,			/* 94 */
    INSN_OP_SIBLING,			/* 95 */
    INSN_OP_PPADDR,			/* 96 */
    INSN_OP_TARG,			/* 97 */
    INSN_OP_TYPE,			/* 98 */
    INSN_OP_SEQ,			/* 99 */
    INSN_OP_FLAGS,			/* 100 */
    INSN_OP_PRIVATE,			/* 101 */
    INSN_OP_FIRST,			/* 102 */
    INSN_OP_LAST,			/* 103 */
    INSN_OP_OTHER,			/* 104 */
    INSN_OP_PMREPLROOT,			/* 105 */
    INSN_OP_PMREPLSTART,			/* 106 */
    INSN_OP_PMNEXT,			/* 107 */
    INSN_OP_PMSTASHPV,			/* 108 */
    INSN_OP_PMREPLROOTPO,			/* 109 */
    INSN_OP_PMSTASH,			/* 110 */
    INSN_OP_PMREPLROOTGV,			/* 111 */
    INSN_PREGCOMP,			/* 112 */
    INSN_OP_PMFLAGS,			/* 113 */
    INSN_OP_PMPERMFLAGS,			/* 114 */
    INSN_OP_PMDYNFLAGS,			/* 115 */
    INSN_OP_SV,			/* 116 */
    INSN_OP_PADIX,			/* 117 */
    INSN_OP_PV,			/* 118 */
    INSN_OP_PV_TR,			/* 119 */
    INSN_OP_REDOOP,			/* 120 */
    INSN_OP_NEXTOP,			/* 121 */
    INSN_OP_LASTOP,			/* 122 */
    INSN_COP_LABEL,			/* 123 */
    INSN_COP_STASHPV,			/* 124 */
    INSN_COP_FILE,			/* 125 */
    INSN_COP_STASH,			/* 126 */
    INSN_COP_FILEGV,			/* 127 */
    INSN_COP_SEQ,			/* 128 */
    INSN_COP_ARYBASE,			/* 129 */
    INSN_COP_LINE,			/* 130 */
    INSN_COP_IO,			/* 131 */
    INSN_COP_WARNINGS,			/* 132 */
    INSN_MAIN_START,			/* 133 */
    INSN_MAIN_ROOT,			/* 134 */
    INSN_MAIN_CV,			/* 135 */
    INSN_CURPAD,			/* 136 */
    INSN_PUSH_BEGIN,			/* 137 */
    INSN_PUSH_INIT,			/* 138 */
    INSN_PUSH_END,			/* 139 */
    INSN_CURSTASH,			/* 140 */
    INSN_DEFSTASH,			/* 141 */
    INSN_DATA,			/* 142 */
    INSN_INCAV,			/* 143 */
    INSN_LOAD_GLOB,			/* 144 */
    INSN_REGEX_PADAV,			/* 145 */
    INSN_DOWARN,			/* 146 */
    INSN_COMPPAD_NAME,			/* 147 */
    INSN_XGV_STASH,			/* 148 */
    INSN_SIGNAL,			/* 149 */
    INSN_FORMFEED,			/* 150 */
    MAX_INSN = 150
};

enum {
    OPt_OP,		/* 0 */
    OPt_UNOP,		/* 1 */
    OPt_BINOP,		/* 2 */
    OPt_LOGOP,		/* 3 */
    OPt_LISTOP,		/* 4 */
    OPt_PMOP,		/* 5 */
    OPt_SVOP,		/* 6 */
    OPt_PADOP,		/* 7 */
    OPt_PVOP,		/* 8 */
    OPt_LOOP,		/* 9 */
    OPt_COP		/* 10 */
};

/* ex: set ro: */