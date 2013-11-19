/*
 *DESC: extra segment storage management  (hybrid models)
 */
#include "alice.h"
#include <task.h>
#include <task_msgs.h>
#include <stdio.h>

#ifdef DB_ESMEM
# define PRINTT
#endif
#include "printt.h"


/*
 * Routines for handling extra segment strings.
 * 
 * getESString() fetches a string from the extra segment into the
 * data segment and returns it.
 *
 * ESString() returns the previously fetched string.
 *
 * setESString() stores a string from the data segment into the extra
 * segment.
 */
char	ESStrBuf[MAX_LEN_LINE]; /* ACK, ANOTHER HUGE BUFFER? */

char *
getESString(where)
treep	where;
{
	return esdsstrcpy(ESStrBuf, where);
}

char *
ESString()
{
	return ESStrBuf;
}

treep
putESString(where, str)
treep	where;
char	*str;
{
	return dsesstrcpy(where, str);
}

treep
allocESString(str)
char	*str;
{
	int	len	= strlen(str);
	treep	p	= esalloc(len + 1);

	dsesstrcpy(p, str);
	return	p;
}

freeESString(str)
treep	str;
{
	esfree(str, strlen(getESString(str)) + 1);
}

treep
dsesstrcpy(dest, src)
treep dest;
char *src;
{
	treep	temp = dest;

	while (@dest++ = *src++)
		;
	return temp;
}

char *
esdsstrcpy(dest, src)
char	*dest;
treep	src;
{
	char	*temp = dest;
	while (*dest++ = @src++)
		;
	return temp;
}

static unsigned ERM_seg;		/* error message segment */
static char *errmsgmem;			/* error message offset */
static int ermsg_count;
static unsigned msgmemsize;		/* size of error message mem */
static unsigned int *ermsgtable;
static int erloaded;


loadErrors(erfile)
FILE *erfile;
{
	char erline[MX_WIN_WIDTH+10];
	register int errnum;
	int blocksneeded;
	char *mmptr;		/* message memory pointer */

	fgets( erline, MX_WIN_WIDTH+9, erfile );
	ermsg_count = atoi( erline );
	msgmemsize = atoi( strchr( erline, '\t' ) + 1 ) + 2;

	blocksneeded = bytesToBlocks(msgmemsize);
	if( truelargest - maxESBlocks > blocksneeded ||
					blocksneeded < secondlargest ) {
		/* grab a fresh segment for error messages */
		ERM_seg = alloc_segment(blocksneeded);
		if( !ERM_seg )
			return 1;
		errmsgmem = 0;
		}
	 else {
		/* use existing memory for error messages */
		ERM_seg = ESSeg;
		errmsgmem = esalloc( msgmemsize );
		if( !errmsgmem )
			return 1;
		}

	set_extra_segment(ERM_seg);

	printt1("errmsgmem=%lx\n", errmsgmem);
	mmptr = errmsgmem;
	/* decision to add errors in data segment ? */
	ermsgtable = (unsigned int *)checkalloc((1+ermsg_count) *
					sizeof( unsigned int ) );
	printt1("ermsgtable=%lx\n", ermsgtable );

	@mmptr++ = 0;
	while( fgets( erline, MX_WIN_WIDTH+9, erfile ) ) {
		register char *msgtext;
		errnum = atoi( erline );

		msgtext = strchr( erline, '\t' );
		/* postincrement to go one past the tab */
		/* greater than at front means debug/bug message */
		if( msgtext++ && msgtext[0] != '>' ) {
			if( msgtext[0] == '#' || msgtext[0] == '/' ) {
				msgtext++;
			RemoveNL( msgtext );
			ermsgtable[ errnum ] = mmptr - errmsgmem;
			while( @mmptr++ = *msgtext++ )
				;
			}
		 else
			ermsgtable[errnum] = 0;
			
		}
	erloaded = TRUE;	/* mark error messages installed */
	if( (mmptr - errmsgmem) > msgmemsize ) {
		fprintf( stderr, "Error message memory overrun, actual size %d", mmptr - errmsgmem );
		exit(1);
		}
	set_extra_segment(ESSeg);
	return 0;
}

char *
getERRstr(errno)
unsigned errno;
{
	int errorindex;

	/* comment out to avoid loading */
	set_extra_segment(ERM_seg);
	if( erloaded && errno <= ermsg_count &&(errorindex=ermsgtable[errno])) {
		esdsstrcpy( ESStrBuf, errmsgmem + errorindex );
		}
	 else 
		sprintf( ESStrBuf, "bug`Unknown Error #%d args %x,%x,%x", errno );

	set_extra_segment(ESSeg);
	return ESStrBuf;
}
