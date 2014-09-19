#if DOMEMCHECK

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <sys/time.h>

#include "memcheck.h"

#define PREFERREDALIGN		8
#define MEMCK_NEED_ALIGN8	1


#define  OSProvided_Malloc(x)     malloc(x)
#define  OSProvided_Free(x)       free(x)
#define  OSProvided_Calloc(x, y)  calloc(x, y)
#define  OSProvided_Realloc(x, y) realloc(x, y)


#define DebugAssert(x) 
#define Assert(x)


/* for 32-bit architecures: */
typedef unsigned long Card32;
typedef unsigned long *pCard32;
typedef long Int32;
typedef unsigned short Card16;
typedef unsigned char Card8;
typedef unsigned char *pCard8;
typedef unsigned short boolean;

#define NUM_MEM_MASK 32
#define NUM_MEM_BLK 32*NUM_MEM_MASK

typedef struct _t_MemoryData {
   void   *address[NUM_MEM_BLK];
   Card32  allocNdx[NUM_MEM_BLK];
   Card32  inUseMask[NUM_MEM_MASK];
   Card32  keepMask[NUM_MEM_MASK];
   char   *fileName[NUM_MEM_BLK];
   Card32 size[NUM_MEM_BLK];
   Card32 lineNo[NUM_MEM_BLK];
   Card32 memStore[NUM_MEM_BLK];
   Card32 numInUse;
   struct _t_MemoryData *next;
} *MemoryData, MemoryDataRec;

static MemoryData memoryData;


static Card32 distAllocNdx = 0;
static Card32 distAllocCount = 0;
static Card32 totalAllocCount = 0;
static Card32 distAllocNdxStopper = 0;

SINGEXPORT int memck_PrintAllAllocFreeCalls = 0; /* print out all allocations/reallocations/frees */

static FILE *logfile = NULL;

/* ----------------------------------------------------------- */

static int MIN(int a, int b)
{
  if (a < b) return a;
  return b;
}

static int MAX(int a, int b)
{
	if (a > b) return a;
	return b;
}

static char *CheckName (char* filename) 
{
#define MEMCKMAXNUMFILENAMES 256
   static char  outOfMemStr[] = "<no memory to store filename>";
   static char *fileNames[MEMCKMAXNUMFILENAMES];
   static int   numNames = 0;

   char *cptr;
   int   i;

   for( cptr=filename; *cptr; ++cptr )
      if( *cptr == '/' ) filename = cptr+1;
   
   for( i=0; i<numNames; ++i )
      if( !strcmp( filename, fileNames[i] ) ) break;
   if( i < numNames ) return fileNames[i];
   if (numNames < (MEMCKMAXNUMFILENAMES - 1))
	   {
	   cptr = fileNames[numNames++] = (char *)OSProvided_Malloc( strlen(filename)+1 );
	   if( cptr ) strcpy( cptr, filename );
	   else       cptr = outOfMemStr;
	   }
   else
       cptr = outOfMemStr;
   return cptr;
}

/* You can call this function from gdb or set the global distAllocNdxStopper
 * directly to stop at a given allocation index with an assertion failure
 * after the memory leak report has detected one and given you an index. */
SINGEXPORT void StopOnAllocation (Card32 stopNum)
{
	distAllocNdxStopper = stopNum;
}

/* 
 * This is a good place to set a breakpoint when we reach the given allocation
   index
 */
static void AllocationBreakStop (Card32 stopNum)
{
	fprintf(stdout,"MemCheck allocator reached allocation #%d\n",stopNum);
}

static boolean StoreAllocation(
    void* ptr, 
    int   size,
    MemCheck_MemStore  memStore,
    char* filename, 
    int   lineno
    )
{
   MemoryData mPtr, *prev;

   /* This debugassert will trip on the given allocation numbered
    * distAllocNdx as * reported in the memory leak report. This is much
    * faster than a conditional * breakpoint. <Gus 9/25/97> */
   if (distAllocNdxStopper != 0 && distAllocNdxStopper <= distAllocNdx) {
   		AllocationBreakStop(distAllocNdx);
		distAllocNdxStopper = 0;
   }
   DebugAssert(distAllocNdxStopper == 0 || distAllocNdxStopper != distAllocNdx);
   prev = &memoryData;
   for( mPtr=memoryData; mPtr; mPtr=mPtr->next ) {
      if( mPtr->numInUse < NUM_MEM_BLK ) {
         int i,j,k;
         void  **addr;
         pCard32 mask = mPtr->inUseMask;
         for( i=0; i<NUM_MEM_MASK; ++i,++mask )
            if( *mask != 0xFFFFFFFF ) break;
         addr = &mPtr->address[i<<5];
         for( j=0; j<32; ++j,++addr )
            if( *addr == NULL ) break;
         k = (i<<5) + j;
         *mask |= ((Card32)1 << j);
         mPtr->keepMask[i] |= ((Card32)1 << j);
         ++mPtr->numInUse;
         *addr = ptr;
         mPtr->allocNdx[k] = distAllocNdx;
         mPtr->fileName[k] = CheckName( filename );
         mPtr->lineNo[k] = lineno;
         mPtr->size[k] = size;
         mPtr->memStore[k] = memStore;
         return 1;
      }
      prev = &mPtr->next;
   }
   *prev = mPtr = (MemoryData)OSProvided_Calloc( sizeof(MemoryDataRec), 1 );
   if( !mPtr ) return 0;
   mPtr->address[0]   = ptr;
   mPtr->allocNdx[0]  = distAllocNdx;
   mPtr->inUseMask[0] = 1;
   mPtr->keepMask[0]  = 1;
   mPtr->fileName[0]  = CheckName( filename );
   mPtr->lineNo[0]    = lineno;
   mPtr->size[0]      = size;
   mPtr->memStore[0] = memStore;
   mPtr->numInUse = 1;
   mPtr->next = NULL;
   return 1;
}


static void ReplaceAllocation (void* ptr1, 
							   void* ptr2,
							   int size, 
							   char* filename, 
							   int lineno)
{
   MemoryData mPtr, *prev;

   prev = &memoryData;
   for( mPtr=memoryData; mPtr; mPtr=mPtr->next ) {
      int i,j,k;
      void **addr;
      pCard32 mask = mPtr->inUseMask;
      pCard32 keep = mPtr->keepMask;
      DebugAssert( mPtr->numInUse > 0 );
      for( i=0; i<NUM_MEM_MASK; ++i,++mask,++keep ) {
         if( *mask == 0 ) continue;
         addr = &mPtr->address[i<<5];
         for( k=i<<5,j=0; j<32; ++j,++k,++addr ) {
            if( *addr == ptr1 ) {
               if( (*addr = ptr2) != NULL ) {
                  mPtr->fileName[k] = CheckName( filename );
                  mPtr->allocNdx[k] = distAllocNdx;
                  mPtr->lineNo[k]   = lineno;
                  mPtr->size[k]     = size;
                  return;
               }
               *mask &= ~((Card32)1 << j);
               *keep &= ~((Card32)1 << j);
               if( --mPtr->numInUse == 0 ) {
                  MemoryData next = mPtr->next;
                  OSProvided_Free( mPtr );
                  *prev = next;
               }
               return;
            }
         }
      }
      prev = &mPtr->next;
   }
   DebugAssert( 0 );
}


static void ResetAllocations (void)
{
   MemoryData mPtr;
   for( mPtr=memoryData; mPtr; mPtr=mPtr->next )
      memset( mPtr->keepMask, 0, NUM_MEM_MASK<<2 );
}


static void PrintAllocations ()
{
   MemoryData mPtr;   int i, j, k;
   int numLeft=0;

   for( mPtr=memoryData; mPtr; mPtr=mPtr->next ) {
      void **addr;
      pCard32 mask = mPtr->inUseMask;
      pCard32 keep = mPtr->keepMask;
      DebugAssert( mPtr->numInUse > 0 );
      for( i=0; i<NUM_MEM_MASK; ++i,++mask,++keep ) {
         if( *mask == 0 || *keep == 0 ) continue;
         addr = &mPtr->address[i<<5];
         for( j=0; j<32; ++j,++addr )
            if( *addr != NULL && (*keep & (1<<j)) != 0  ) ++numLeft;
      }
   }
   if( !numLeft ) return;

   if (logfile) 
	   fprintf( logfile, "WARNING: Possible memory leak; %d items left allocated\n", numLeft );
   for( mPtr=memoryData; mPtr; mPtr=mPtr->next ) {
      void **addr;
      pCard32 mask = mPtr->inUseMask;
      pCard32 keep = mPtr->keepMask;
      DebugAssert( mPtr->numInUse > 0 );
      for( i=0; i<NUM_MEM_MASK; ++i,++mask,++keep ) {
         if( *mask == 0 || *keep == 0 ) continue;
         addr = &mPtr->address[i<<5];
         for( k=i<<5,j=0; j<32; ++j,++k,++addr ) {
            if( *addr == NULL || (*keep & (1<<j)) == 0  ) continue;

		 if (logfile) 
			 fprintf(logfile,
					 "  (Store=%d)\t 0x%x size=%d (ndx=%d) file=%s@%d\n", 
					 mPtr->memStore[k],
					 *addr,
					 mPtr->size[k],
					 mPtr->allocNdx[k],
					 mPtr->fileName[k], mPtr->lineNo[k] );
         }
      }
   }
   if (logfile)   fflush(logfile);
}

/* ----------------------------------------------------------- */


/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */
/* Mem Store buffers are allocated in per_store_Data Blocks */
typedef struct t_PerstoreData *PPerstoreData;  
typedef struct t_PerstoreData  
{
    pCard8      data;      /* use char* for easy pointer math */
    Int32      size;      /* original size of the block */
    Int32      left;      /* contiguious bytes left over in this block */
    Int32      discard;   /* this block can be freed IF discard+left == size */
    PPerstoreData next;
    PPerstoreData prev;
}PerstoreData;

typedef struct t_MemStore
{
    PerstoreData    head[1];

    /* statistics for debugging */
    int  allocated;     /* Actual allocated size for this memStore - including freed */
    int  used;          /* size used AND re-used */
    int  freed;         /* allocated - freed = size used for this memStore */
    int  maxInUse;      /* Maximal mem size in use for this memStore: MAX(allocated-freed) */
    int  maxBlock;      /* max block size */
    int  numBlocks;     /* blocks left over */
    int  reqBlocks;     /* num of blocks requested - inlcuding freed ones */

}MemStore, *PMemStore;


#define PD_BLOCK_MIN_SIZE       0x4000    /* default min size */
#define PD_BLOCK_MAX_SIZE       0xFFF0    /* Max size allowed from MemStores */

/* All blocks allocated are in one of the stores */
static MemStore gMStores[memck_store_maxMemStore];

/*  2 Giga bytes is memck_*alloc()'s max size 
 *  so the size in ((size << 1) | MEMCK_OSMALLOC_BIT) can be recovered
 */
#define MEMCK_MAX_SIZE         0x7FFFFFF0
#define MEMCK_OSMALLOC_BIT     0x00000001

/*
 * Every memck_*alloc()'d ptr has an Card32 tag in its front
 * each buffer looks like this :
 *           (HEAD)(UserPtr)(TAIL)
 *
 * If the pointer is from OSProvided_*alloc():
 *   (HEAD) has MEMCK_OSMALLOC_BIT set which will never be set
 *    by memstore_*alloc()
 *
 * If the pointer is from memstore_*alloc():
 *   (HEAD) is divided into two Card16 (DS)(SZ) where
 *    SZ = size of (HEAD + UserPtr + TAIL), it is always even.
 *    DS = DistanceToBlockHeader, its maximal value is
 *       PD_BLOCK_MAX_SIZE - SZ
 *
 * In Release build. (TAIL) is not included
 *
 *  For Sparc Solaris we have the additional requirement that all malloc
 *  pointers returned to the client must be 8-byte aligned. This is to
 *  allow the client to store doubles in the segment (if the ptr is not
 *  8-byte aligned a BUS error will happen). So for Solaris we wrap the
 *  entire buffer like this:
 *
 *  1) If the client pointer is unaligned (only 4-byte aligned):
 *
 *     Base malloc pointer
 *     |
 *     |        Unaligned unwrapped client pointer
 *     |        |
 *     |        |        Aligned wrapped client pointer
 *     |        |        |
 *     V        V        V
 *       4 byte   4 byte                                   4 byte   4 byte
 *     +--------+--------+-------------------------------+--------+--------+
 *     |  HEAD  |   = 1  | Client segment                | unused |  TAIL  |
 *     +--------+--------+-------------------------------+--------+--------+
 *
 *                       |<--- MEMCK_GetInnerClientSize --->|
 *
 *              |<-------------- MEMCK_GetClientSize --------------->|
 *
 *
 *  2) If the client pointer is aligned (8-byte aligned):
 *
 *     Base malloc pointer
 *     |
 *     |        Aligned unwrapped client pointer
 *     |        |
 *     |        |                 Aligned wrapped client pointer
 *     |        |                 |
 *     V        V                 V
 *       4 byte   4 byte   4 byte                                   4 byte
 *     +--------+--------+--------+-------------------------------+--------+
 *     |  HEAD  |  = 2   |   = 2  | Client segment                |  TAIL  |
 *     +--------+--------+--------+-------------------------------+--------+
 *
 *                                |<--- MEMCK_GetInnerClientSize --->|
 *
 *              |<-------------- MEMCK_GetClientSize --------------->|
 *
 *  The 4-byte unsigned integers between the base pointer and the client
 *  pointer indicate whether or not the base pointer is unaligned.
 *
 *  Exported functions:
 *
 *      memck_mallocinternal
 *      memck_callocinternal
 *      memck_freeinternal
 *      memck_reallocinternal
 *
 *  Internal call-hierarchy:
 *
 *      memck_callocinternal
 *        |
 *        `---> memck_mallocinternal
 *
 *      memck_reallocinternal
 *        |
 *        `---> memstore_reallocinternal
 *                |
 *                `---> memck_mallocinternal
 *                `---> mekck_freeinternal
 */


#if MEMCK_NEED_ALIGN8

#define MEMCK_IS_ALIGNED8(ptr) ( ( ((Card32)(ptr)) % 8 ) == 0 )
#define MEMCK_PADDING_ALIGN8  (sizeof(Card32)*2)

/*------------------------------------------*/
#define MEMCK_WRAP8(wPtr)                       \
                                             \
  {                                          \
     if (wPtr)                               \
     {                                       \
        if ( MEMCK_IS_ALIGNED8(wPtr) )          \
        {                                    \
           *((pCard32)wPtr) = 2;            \
            ((pCard32)wPtr)++;              \
           *((pCard32)wPtr) = 2;            \
            ((pCard32)wPtr)++;              \
        }                                    \
        else                                 \
        {                                    \
           *((pCard32)wPtr) = 1;            \
            ((pCard32)wPtr)++;              \
        }                                    \
        DebugAssert( MEMCK_IS_ALIGNED8(wPtr) ); \
     }                                       \
  }

/*--------------------------------------------*/
#define MEMCK_UNWRAP8(wPtr)                       \
                                               \
  {                                            \
     DebugAssert(wPtr);                        \
     DebugAssert( MEMCK_IS_ALIGNED8(wPtr) );      \
     ((pCard32)wPtr)--;                       \
     if ( *((pCard32)wPtr) == 2 )             \
     {                                         \
        ((pCard32)wPtr)--;                    \
        DebugAssert( *((pCard32)wPtr) == 2 ); \
     }                                         \
     else                                      \
     {                                         \
        DebugAssert( *((pCard32)wPtr) == 1 ); \
     }                                         \
  }

#else

#define MEMCK_PADDING_ALIGN8  ( 0 )
#define MEMCK_WRAP8(wPtr)
#define MEMCK_UNWRAP8(wPtr)

#endif

#define MEMCK_HEAD_SIZE  (sizeof(Card32))

/*------------------------------------------------*/
/* From internal data pointer to clients' pointer */
/*------------------------------------------------*/

#define MEMCK_MakeClientPtr(basePtr) ((void *)((pCard8)(basePtr) + MEMCK_HEAD_SIZE) )

/*------------------------------*/
/* From client's pointer to ... */
/*------------------------------*/

#define MEMCK_BasePtr(clientPtr)     ( (void *)((pCard8)(clientPtr) - MEMCK_HEAD_SIZE) )
#define MEMCK_GET_HEAD32(clientPtr)  ( *((pCard32)MEMCK_BasePtr(clientPtr))            )
#define PD_GET_SZ16(clientPtr)       (  MEMCK_GET_HEAD32(clientPtr) & 0x0000FFFF        )
#define PD_GET_DS16(clientPtr)       ( (MEMCK_GET_HEAD32(clientPtr) & 0xFFFF0000) >> 16 )
#define MEMCK_IsOSmalloc(clientPtr)  (  MEMCK_GET_HEAD32(clientPtr) & MEMCK_OSMALLOC_BIT   )
#define PD_GetBlock(clientPtr)       ( (pCard8)(clientPtr) - PD_GET_DS16(clientPtr) )

/*
** size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES, actual saved
** (HEAD) is either ( size<<1 | 1) or (DS)(SZ) depends on if Blk==NULL
*/

#define MEMCK_SetTotalSize(clientPtr, Blk, siz) \
                                             \
   MEMCK_GET_HEAD32(clientPtr) =                \
      (                                      \
         (Blk == NULL) ?                     \
          ( (((Card32)(siz)) << 1) | MEMCK_OSMALLOC_BIT                           ) : \
          (   (Card32)(siz) | ( ((Card32)(clientPtr) - (Card32)(Blk)) << 16) )   \
      )

#define MEMCK_GetTotalSize(clientPtr) \
    ( MEMCK_IsOSmalloc(clientPtr) ? (MEMCK_GET_HEAD32(clientPtr) >> 1) : PD_GET_SZ16(clientPtr) )

#define MEMCK_AlignSize(M_Size) \
    ( ( ( ( (M_Size)+PREFERREDALIGN-1)/PREFERREDALIGN) * PREFERREDALIGN ) + MEMCK_PADDING_ALIGN8 )


/*
** Guard the tail for debugging purpose - because a corrupted
** tail is not easily spotted if it is from the big block
*/

#define MEMCK_TAIL_SIZE    (sizeof(Card32))
#define MEMCK_EXTRA_BYTES  (MEMCK_HEAD_SIZE + MEMCK_TAIL_SIZE)
#define MEMCK_TAIL_GUARD   0xADBEADBE       /* ADoBE ADoBE */
#define MEMCK_TAIL_FREED   0xFEEDFEED       /* FrEED FrEED */

#define MEMCK_GetClientSize(clientPtr) ( MEMCK_GetTotalSize(clientPtr) - MEMCK_EXTRA_BYTES )

#if MEMCK_NEED_ALIGN8
#define MEMCK_GetInnerClientSize(clientPtr) ( MEMCK_GetClientSize(clientPtr) - MEMCK_PADDING_ALIGN8 )
#endif

/*---------------------------------------*/
/* From client's Ptr to its tail's value */
/*---------------------------------------*/

#define MEMCK_GET_TAIL(clientPtr)  *((pCard32)((pCard8)(clientPtr) + MEMCK_GetClientSize(clientPtr)))

/*----------------------------*/
/* Guard client's pointer Ptr */
/*----------------------------*/

#define MEMCK_GuardTail(clientPtr)  MEMCK_GET_TAIL(clientPtr)  = (Card32)MEMCK_TAIL_GUARD 
#define MEMCK_FreedTail(clientPtr)  MEMCK_GET_TAIL(clientPtr)  = (Card32)MEMCK_TAIL_FREED 
#define MEMCK_BadTail(clientPtr)   (MEMCK_GET_TAIL(clientPtr) != (Card32)MEMCK_TAIL_GUARD)



/* -------------------------------------------------- */
/* ------------ pointers from MemStores ------------- */
/* -------------------------------------------------- */
static void * memstore_mallocinternal(
    int             len,
    MemCheck_MemStore   index,
	char    *filename,
	int     lineno
    )
{
    PMemStore pms = &(gMStores[index]);
    PPerstoreData ppd;
    int     size;
    pCard8 pByte = NULL;

    pms->used += len;
    DebugAssert(len < PD_BLOCK_MAX_SIZE + 4 );

    /* The buffer we return has some extra bytes for internal use */
    len += MEMCK_EXTRA_BYTES ;
    /* Make len multiple of 4 */
    len = ( (len + 3) / 4 ) * 4;

    /* search left overs */
    for( ppd = pms->head[0].next; ppd; ppd = ppd->next ) 
    {
        if( ppd->left < len ) 
            continue;
        pByte = ppd->data + ppd->size - ppd->left ;
        ppd->left -= len;
        break;
    }

    if (pByte == NULL)
    {
        /* need a new block for this store */
        if (len < PD_BLOCK_MIN_SIZE )
            size = MAX(2 * len, PD_BLOCK_MIN_SIZE) ;
        else
            size = len;
        size += sizeof(PerstoreData);
        /* One OSProvided_Malloc() allocates both the structure and its data */
        ppd = (PPerstoreData)OSProvided_Malloc( size );
        if( !ppd ) 
            return NULL;

        ppd->data = (pCard8) ( (pCard8)ppd + sizeof(PerstoreData) );
        ppd->size = 
        ppd->left = size - sizeof(PerstoreData) ; 
        ppd->discard = 0;
        ppd->next = pms->head[0].next;
        if (pms->head[0].next)
            pms->head[0].next->prev = ppd;
        ppd->prev = &(pms->head[0]);
        pms->head[0].next = ppd;

        pByte = ppd->data ;
        ppd->left -= len;

        pms->numBlocks++;
        pms->reqBlocks++;
        pms->allocated += size;
        if (size > pms->maxBlock)
            pms->maxBlock = size;
        if ((pms->allocated - pms->freed) > pms->maxInUse)
            pms->maxInUse = (pms->allocated - pms->freed);
    }

    if (pByte)
    {
        /* Now tag pByte with length and block -  size include the extra bytes */
        pByte = (pCard8) MEMCK_MakeClientPtr(pByte);
        /* size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES */
        MEMCK_SetTotalSize(pByte, ppd, len);
        MEMCK_GuardTail(pByte);
    }
    return ((void *) pByte);
}


static MemCheck_MemStore FindMemStore(
    PPerstoreData  ptr
    )
{
    PPerstoreData  ppd;
    int index;
    for (index = memck_store_default; index < memck_store_maxMemStore; index++)
    {
        ppd = gMStores[index].head[0].next;
        while (ppd)
        {
            if ( (Card32)ptr > (Card32)ppd &&
                 (Card32)ptr < (Card32)(ppd->data + ppd->size) )
            {
                Assert(ppd == (PPerstoreData)PD_GetBlock(ptr) );
                return (MemCheck_MemStore)index;
            }
            ppd = ppd->next;
        }
    }
    return memck_store_maxMemStore;
}


static boolean memstore_remove_from_store(
    PPerstoreData   ppd,
    MemCheck_MemStore   index 
    )
{
    PMemStore pms = &(gMStores[index]);
    if ( ppd->discard + ppd->left == ppd->size)
    {
        pms->numBlocks--;
        pms->freed += ppd->size + sizeof(PerstoreData);
        if (ppd->next)
            ppd->next->prev = ppd->prev;
        if (ppd->prev)
            ppd->prev->next = ppd->next;
        OSProvided_Free((void *)ppd);
        return 1;
    }
    return 0;
}


/* ptr must be memck_per_*alloc()-ed */
static void memstore_freeinternal(
    void *ptr
    )
{
    PPerstoreData ppd;
    Int32       size;
    ppd = (PPerstoreData)PD_GetBlock(ptr);
    /* size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES */
    size = MEMCK_GetTotalSize(ptr);
    if (MEMCK_BasePtr(ptr) == (void *) (ppd->data + ppd->size - ppd->left - size) )
    {
        /* Coalesce to left space */
        ppd->left += size;
    }
    else
    {
        /* don't bother to coalesce with other free items, just remember the size freed */
        ppd->discard += size;
        /* remove the block if we can */
        if ( ppd->discard + ppd->left == ppd->size)
        {
            MemCheck_MemStore   index = FindMemStore((PerstoreData*)ptr);
            memstore_remove_from_store(ppd, index);
        }
    }
}

#if (MEMCK_NEED_ALIGN8 == 0)

static void *memstore_reallocinternal(
    void        *old_ptr, 
    int          new_size,
	char    *filename,
	int     lineno
    )
{
    void *new_ptr;
    MemCheck_MemStore index = FindMemStore((PerstoreData*)old_ptr);
    DebugAssert(index < memck_store_maxMemStore );
    /* realloc from Store index */
    new_ptr =  memck_mallocinternal( new_size, index , filename, lineno);
    if (new_ptr)
    {
        /* size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES */
        int old_size = MEMCK_GetTotalSize(old_ptr);
        old_size -= MEMCK_EXTRA_BYTES;

        DebugAssert (old_size < gMStores[index].maxBlock );
        DebugAssert (old_size >= 0);

        bcopy(old_ptr, new_ptr, MIN(old_size, new_size) );
        memck_freeinternal(old_ptr , filename, lineno);
    }
    return (new_ptr);
}

#endif /* MEMCK_NEED_ALIGN8 == 0 */

static void FreeOneMemStore(
    MemCheck_MemStore   index 
    )
{
    PMemStore pms = &(gMStores[index]);
    PPerstoreData ppd, next;

    if (pms->head[0].next == NULL)
        return;

    /* Free the list */
    ppd = pms->head[0].next;
    while (ppd)
    {
        next = ppd->next;
        memstore_remove_from_store(ppd, index);
        ppd = next;
    }

    /* These are report for current job - reset for next job*/
    pms->maxInUse = 0;
    pms->allocated = 0;
    pms->used = 0;
    pms->freed = 0;
}


/* -------------------------------------------------- */
/* ----------------- SINGEXPORT functions      ---------- */

SINGEXPORT int memck_Startup(char *logfilePathName)
{
	time_t timer;

	logfile = fopen(logfilePathName, "a+");
	if (logfile)
	{
	time (&timer);
	fprintf(logfile, "========= Started: %s\n", ctime(&timer));
	}
	return (logfile == NULL);
}

SINGEXPORT void memck_Shutdown(void)
{
	time_t timer;
    int index;
    for (index = memck_store_default; index < memck_store_maxMemStore; index++)
        FreeOneMemStore((MemCheck_MemStore)index);
	if (logfile)
	{
	time (&timer);
	fprintf(logfile, "========== Finished: %s\n", ctime(&timer));
	}

	fclose(logfile);
	logfile = NULL;
}


/* ----------------------------------------------------------- */
SINGEXPORT void *memck_mallocinternal(
    int             size, 
    MemCheck_MemStore   index,
	char    *filename,  
	int     lineno
    )
{
    void * result;

    DebugAssert(index >= memck_store_default);
    DebugAssert(index <  memck_store_maxMemStore);

    /* malloc()/mc_Malloc() also allows 0 byte allocation */
    if (size < 0 || size > MEMCK_MAX_SIZE )
        return NULL;

    size = MEMCK_AlignSize(size);

    if ((index > memck_store_default) && size < PD_BLOCK_MAX_SIZE )
    {
        result = memstore_mallocinternal(size, index, filename, lineno);
        DebugAssert( !MEMCK_IsOSmalloc(result));
    }
    else
    {
        /* old way, but with MEMCK_EXTRA_BYTES more bytes allocated */
        result = OSProvided_Malloc(size + MEMCK_EXTRA_BYTES );
        /* Convert to user's pointer, skip the header bytes and tag it */
        if (result)
        {
            result = MEMCK_MakeClientPtr(result);
            /* size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES */
            MEMCK_SetTotalSize(result, NULL, size + MEMCK_EXTRA_BYTES );
            MEMCK_GuardTail(result);
        }
    }

    if( !result ) 
        return NULL;

    if( !StoreAllocation( result, size, index, filename, lineno) )
    {    
        if( MEMCK_IsOSmalloc(result) )
            OSProvided_Free(MEMCK_BasePtr(result));
        else
            memstore_freeinternal(result);
        return NULL;
    }

    ++distAllocCount;
    ++distAllocNdx;
    if (memck_PrintAllAllocFreeCalls)
    {
	if (logfile)
        fprintf(logfile,
				"Malloc 0x%x\t siz=%d tcnt=%d cnt=%d file=%s@%d\n",
				result,
				size,
				++totalAllocCount,
				distAllocCount
				, filename, lineno
				);
	if (logfile) fflush(logfile);
    }

    MEMCK_WRAP8(result);

    return (result);
}


/* ----------------------------------------------------------- */
SINGEXPORT void *memck_callocinternal(
    int             count, 
    int             size,
    MemCheck_MemStore   index,
	char    *filename,
	int     lineno
    )
{
    void * result;

    DebugAssert(index >= memck_store_default);
    DebugAssert(index <  memck_store_maxMemStore);

    result = memck_mallocinternal( count * size, index , filename, lineno);
    if (result)
        memset(result, 0, count * size);
    return result;
}


/* ----------------------------------------------------------- */
SINGEXPORT void memck_freeinternal(
    void    *ptr,
	char    *filename,
	int     lineno
    )
{
    /* Just ignore if ptr is NULL, but memck_free used to Assert() */
    DebugAssert(ptr != NULL);

    if ( ptr == NULL )
        return;

    MEMCK_UNWRAP8(ptr);

    ReplaceAllocation( ptr, 0, 0, 0, 0 );
    --distAllocCount;
    if (MEMCK_BadTail(ptr))
    {
	if (logfile)
		{
		fprintf(logfile, "WARNING: pointer 0x%x had been over written! Freed in file=%s@%d\n", ptr, filename, lineno);
		fflush(logfile);
		}
        DebugAssert(0);
     }
    else
    {
        /* Mark tail to different value, so we can trap multiple 
           OSProvided_Free() on same pointer */
        MEMCK_FreedTail(ptr);
    }

    if (memck_PrintAllAllocFreeCalls)
    {
	if (logfile)
		{
		fprintf(logfile,
				"Free 0x%x\t cnt=%d file=%s@%d\n",
				ptr,
				distAllocCount,
				filename, lineno
				);
		fflush(logfile);
		}
    }

    if (MEMCK_IsOSmalloc(ptr))
    {
        OSProvided_Free(MEMCK_BasePtr(ptr));
    }
    else
    {
        memstore_freeinternal(ptr);
    }
}


/* ----------------------------------------------------------- */
SINGEXPORT void *memck_reallocinternal(
    void   *old_ptr, 
    int     new_size,
	char    *filename,  
	int     lineno
    )
{
#if MEMCK_NEED_ALIGN8

    int old_size;
    MemCheck_MemStore myIndex;
    void *new_ptr;
    void *tmp_ptr = old_ptr;

    MEMCK_UNWRAP8(tmp_ptr);

    if (MEMCK_IsOSmalloc(tmp_ptr))
       myIndex = memck_store_default;
    else
    {
        myIndex = FindMemStore((PerstoreData*)tmp_ptr);
        DebugAssert(myIndex < memck_store_maxMemStore );
    }

    old_size = MEMCK_GetInnerClientSize(tmp_ptr);

    new_ptr = memck_mallocinternal( new_size, myIndex , filename, lineno);
    if (new_ptr == NULL)
       return (NULL);

    memcpy(new_ptr, old_ptr, MIN(old_size, new_size) );
    memck_freeinternal(old_ptr, filename, lineno);
    return (new_ptr);

#else /* !MEMCK_NEED_ALIGN8 */

    new_size = MEMCK_AlignSize(new_size);

    if (MEMCK_IsOSmalloc(old_ptr))
    {
        void *new_ptr;
        /* old way, but with MEMCK_EXTRA_BYTES more bytes allocated */
        new_ptr = OSProvided_Realloc(MEMCK_BasePtr(old_ptr), new_size + MEMCK_EXTRA_BYTES);
        if (new_ptr) 
        {
            new_ptr = MEMCK_MakeClientPtr(new_ptr);
            /* size set by MEMCK_SetTotalSize() inlcudes MEMCK_EXTRA_BYTES */
            MEMCK_SetTotalSize(new_ptr, NULL, new_size + MEMCK_EXTRA_BYTES );
            MEMCK_GuardTail(new_ptr);
        }
        if (new_ptr == NULL)
            return NULL;

		ReplaceAllocation( old_ptr, new_ptr, new_size, filename, lineno);
        ++distAllocNdx;
        if (new_ptr != old_ptr)
        {
            if (memck_PrintAllAllocFreeCalls)
            {
			if (logfile)
				{
				fprintf(logfile,
						"Realloc 0x%x\t cnt=%d file=%s@%d\n",
						old_ptr,
						--distAllocCount,
						filename, lineno
						);
				fprintf(logfile,
						"==> 0x%x\t siz=%d cnt=%d file=%s@%d\n",
						new_ptr,
						new_size,
						++distAllocCount
						, filename, lineno
						);
				fflush(logfile);
				}
            }
        }
	return (new_ptr);
    }
    else
    {
	return ( memstore_reallocinternal(old_ptr, new_size, filename, lineno) );
    }

#endif /* MEMCK_NEED_ALIGN8 */

}

/* ----------------------------------------------------------- */


SINGEXPORT void memck_ResetMemLeakDetection (void)
{
 distAllocCount = 0;
 distAllocNdx = 0;
 ResetAllocations();
}

SINGEXPORT void memck_CheckMemLeaks (void)
{
 PrintAllocations(); 
 memck_ResetMemLeakDetection();
}

#else
static int placeholder(void)
	{
	return 0;
	}
#endif
