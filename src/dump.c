#include "alice.h"
#include "flags.h"
#ifdef PROC_INTERP
#include "process.h"
#endif

/*
 * Routines to print out a sample tree
 */

static int indent = 0;
static treedump(), do_indent(), sym_dump();

dump(loc)
nodep loc;
{
#ifdef DB_DUMP
#ifndef PARSER
	extern int can_dump;
	if (!tracing)
		return;
	fprintf(dtrace, "\ndump of tree\n" );
	if( can_dump ) {
		indent = -1;
		treedump(loc);
		}
	 else {
		fprintf( dtrace, "Sorry, tree not created yet\n" );
		}
	fflush(dtrace);

#endif PARSER
#endif DB_DUMP
}

#ifdef DB_DUMP
#ifndef PARSER
static
treedump(xthenode)
nodep xthenode;          /* current node being listed */
{
	register nodep thenode;
        register int nk;
	int kidnum;
	char *nname;
	thenode = xthenode;
	stackcheck();

	fflush(dtrace);	/* DEBUG */
#ifdef Nodef
	if (!saneptr(thenode)) {
		fprintf(dtrace, "dump: insane pointer %x\n", thenode);
		return;
	}
#endif Nodef
	++indent;
	do_indent();
	if (is_a_stub(thenode)) {
		fprintf(dtrace, "STUB at %lx : %s\n", (long)thenode,
#ifdef PROC_INTERP
			"Unknown"
#else
			classname((int)kid1(thenode))
#endif
			);
		--indent;
		return;
	}
#ifdef SMALL_TPL
	nname = "No Names";
#else
	nname = NodeName(ntype(thenode));
#endif

	fprintf(dtrace, "node=%lx, type %d=%s: ",
		(long)thenode, ntype(thenode), nname ? nname : "Blank" );
	if( ntype_info(ntype(thenode)) & F_SYMBOL )
		sym_dump( sym_kid( thenode ) );
        kidnum = 0;

        if( is_a_list(thenode) ) {
		int i;
		fprintf(dtrace, "parent %lx\n", (long)tparent(thenode) );
		for( i = 0; i < reg_kids( LCAST thenode ); i++ ) {
			do_indent();
                        treedump( node_kid( thenode, i ) );
                }
        } else {
		fprintf(dtrace, "flags %x, parent %lx, kids:",
			node_flag(thenode), (long)node_parent(thenode));
		nk = full_kids(ntype(thenode));
		for (kidnum=0; kidnum<nk; kidnum++)
			fprintf(dtrace, " #%d: %lx,", kidnum, (long)node_kid(thenode,kidnum));
		nk = reg_kids(thenode);
		fprintf(dtrace, "\n");
#ifndef Nodef
		switch (ntype(thenode)) {
		nodep np;
		case N_T_COMMENT:
		case N_CON_INT:
		case N_CON_REAL: {
			char *st;
			do_indent();
			st = getESString(str_kid(0,thenode));
			if (st)
				fprintf(dtrace, "Text: %s\n", st);
			else
				fprintf(dtrace, "stub.\n");
			break;
			}
		case N_CON_STRING:
			do_indent();
			np = kid1(thenode);
			if (np)
				fprintf(dtrace, "constant text: %s\n",
						(char *)np + STR_START +1);
			else
				fprintf(dtrace, "stub.\n");
			break;
		case N_ID:
			do_indent();
			np = kid1(thenode);
			if (np)
				fprintf(dtrace, "symbol: %s\n", sym_name((symptr)np));
			else
				fprintf(dtrace, "stub.\n");
			break;
		case N_DECL_ID:
			do_indent();
			np = thenode;
			if (np)
				fprintf(dtrace, "declared symbol: %s\n", sym_name((symptr)np));
			else
				fprintf(dtrace, "stub.\n");
			break;
		}
#endif
		for (kidnum=0; kidnum<nk; kidnum++)
			treedump(node_kid(thenode,kidnum));
	}
	--indent;
}

static
do_indent()
{
        register int tabc;

        tabc = indent;
        while( tabc-- )
		fprintf(dtrace, "  ");
}

static
sym_dump(table)
nodep table;
{
	register symptr ptr;	/* pointer to move in the table */


	ptr = (symptr) kid1(table);
	/*fprintf(dtrace," Dump up symbol table at %x\n", table );*/
	/* search all symbols in the block for the same pointer */
	fprintf(dtrace, "\n");
	while( ptr ) {
		fprintf( dtrace, "%lx(%s) - type %d typetree %lx, scope %d, size %d, offset/value %lx, flags %x\n",
			(long)ptr,
			sym_name(ptr), sym_dtype(ptr), (long)sym_type(ptr),
			sym_scope(ptr), sym_size(ptr), (long)sym_value(ptr),
			sym_mflags(ptr));
		ptr = sym_next(ptr);
		}
}
#endif PARSER
#endif DB_DUMP

#ifdef CHECKSUM

/* size in words of checksum region */
#define CHECK_SIZE 64
/* 1 fourth of memory */
#define CHECK_BLOCKS 128
/* announce if dirtied after this many clean cycles */
#define CH_THRESHOLD 10
/* this many dirties less than 4 */
#define ALW_DIRTY 25

static unsigned csums[CHECK_BLOCKS];
static unsigned char clean_count[CHECK_BLOCKS];
static unsigned char always_dirty[CHECK_BLOCKS];
static unsigned gcounter=0;

csuminit(){
	int i;
	int sumc;
	unsigned int *p;
	unsigned sum;

	p = 0;
	for( i = 0; i < CHECK_BLOCKS; i++ ) {
		clean_count[i] = CH_THRESHOLD;
		always_dirty[i] = 0;
		sum = 0;
		/* p = &((unsigned *)0)[i * CHECK_SIZE]; */
		sumc = CHECK_SIZE;
		while( sumc-- )
			sum += *p++;
		csums[i] = sum;
		}
}

cscheck()
{
	int i, sumc;
	unsigned sum;
	int reported = FALSE;
	char warn[1000];
	unsigned *p;

	p = 0;
	gcounter++;
	strcpy( warn, "" );

	for( i = 0; i < CHECK_BLOCKS && p < (unsigned *)&p; i++ ) {
		if( always_dirty[i] >= ALW_DIRTY )
			p += CHECK_SIZE;
		 else {
			sumc = CHECK_SIZE;
			sum = 0;
			while( sumc-- )
				sum += *p++;
			if( csums[i] != sum ) {
				/* checksum differs */
				if( clean_count[i] >= CH_THRESHOLD ) {
					/* report failure */
					    sprintf(warn+strlen(warn),
						"%x:%d ",i*CHECK_SIZE
						* sizeof(int), clean_count[i]);
						reported = TRUE;
					}
				 else{	/* mark it gets dirty a lot */
					if( clean_count[i] < 4 )
						always_dirty[i]++;
					if( gcounter > 2 && gcounter < 30 )
						always_dirty[i] = ALW_DIRTY;
					}
				clean_count[i] = 0;
				csums[i] = sum;
				}
			 else {
				if( clean_count[i] < 250 )
					clean_count[i]++;
				}
			}
		}
	if( reported )
		warning(ER(289,"D:%s"), warn );
}

#endif
