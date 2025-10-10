#ifndef INCGUARD_aM91R0ljMWoqtn7F
#define INCGUARD_aM91R0ljMWoqtn7F
#if 0

  Qntan - A C module separations framework

  http://git.hiddenalpha.ch/qntan.git/tree/README.txt

#endif

#include <stddef.h>  /* eg: size_t */
#include <stdint.h>  /* eg: uintptr_t */

#if __cplusplus
extern "C" {
#endif










/*
 * "Cls" aka "Closure". The userdata type to pass around for callbacks etc.  */
#define Qntan_Cls uintptr_t
#define QNTAN_CLS(OBJ) ((Qntan_Cls)OBJ)










#if 0 /* TODO any need for fwd decls? */
struct Qntan_EvLoop;
struct Qntan_Mallocator;
struct Qntan_MemArena;
struct Qntan_Executor;
struct Qntan_IoMux;
struct Qntan_File;
struct Qntan_Process;
struct Qntan_HshTbl;
struct Qntan_HshTbl_Cursor;
struct Qntan_Socket;
struct Qntan_TarEnc;
struct Qntan_TarEnc_Hdr;
struct Qntan_TarDec;
struct Qntan_TarDec_Hdr;
struct Qntan_Networker;
struct Qntan_CsvDec;
struct Qntan_CsvDec_Mentor;
struct Qntan_CsvIStream;
struct Qntan_CsvIStream_Mentor;
struct Qntan_JsonEnc;
struct Qntan_JsonDec;
struct Qntan_JsonDec_Mentor;
struct Qntan_JsonTreeDec_JsonNode;
struct Qntan_JsonTreeDec;
struct Qntan_XmlDec;
struct Qntan_XmlDec_Mentor;
struct Qntan_JavaBytecodeParser;
struct Qntan_JavaBytecodeParser_Mentor;
struct Qntan_CamtAccount;
struct Qntan_CamtTransaction;
struct Qntan_CamtDec_Mentor;
struct Qntan_CamtDec;
#endif










struct Qntan_EvLoop {
	/*
	 * enque a task to the EvLoop. See also Qntan_ThreadPool.enque(). */
	void (*enque)(
		struct Qntan_EvLoop**, void(*fn)(Qntan_Cls), Qntan_Cls );
	/*
	 * TODO write doc */
	void (*addAwaitToken)( struct Qntan_EvLoop** );
	/*
	 * TODO write doc */
	void (*delAwaitToken)( struct Qntan_EvLoop** );
	/*
	 * Process reactor tasks until task queue gets drained.
	 * TODO document return value */
	int  (*runUntilPause)( struct Qntan_EvLoop** );
	/*
	 * Process reactor tasks until event loop is signalized to end. */
	void (*runUntilDone)( struct Qntan_EvLoop** );
#if 0
TODO  /*
TODO   * TODO that should be something like a 'Scheduler' or similar. */
TODO  void (*setTimeoutMs)(
TODO      struct Qntan_EvLoop**, struct Garbage_ThreadPool**, int delayMs,
TODO      void(*fn)(int eno, Qntan_Cls), Qntan_Cls );
#endif
	/*
	 * returns NON-zero, if called from the EvLoopThread. This is intended for
	 * assertions, to check that one is running on the expected thread. */
	int (*isEvLoopThread)( struct Qntan_EvLoop** );
};










struct Qntan_Mallocator {
	/*
	 * Basic idea is
	 * "https://pubs.opengroup.org/onlinepubs/9699919799/functions/realloc.html".
	 * But keep these details in mind:
	 *
	 * Errors are reported via 'errno' as defined by 'realloc' and related
	 * functions.
	 *
	 * In contrast to the traditional funcs, this API requires to also
	 * pass in the size of 'oldPtr'. If you disagree with this decision you
	 * should read "https://nullprogram.com/blog/2023/12/17/".
	 *
	 * Example "allocate space for an object":
	 *   struct Foo *foo = (*m)->realloc(m, NULL, 0, sizeof*foo)
	 *   if( !foo ){ handle errno... }
	 *
	 * Example "free foo":
	 *   (*m)->realloc(m, foo, sizeof*foo, 0)
	 *
	 * Example "enlarge foo from 42 to 1042":
	 *   void*tmp = (*m)->realloc(m, foo, 42, 1042)
	 *   if( !tmp ){ handle errno... }
	 *   foo = tmp;
	 */
	void* (*realloc)(
		struct Qntan_Mallocator**, void*oldPtr, size_t oldPtr_sz, size_t newPtr_sz );
};










/*
 * Let's say, a special kind of mallocator. Traditionally on a mallocator, caller
 * is responsible to ensure alloc/free pairs of calls to prevent memory leaks.
 * On an arena in contrast, individual 'free' for every alloc is NOT needed, as
 * all the allocated space will be free'd at the end of the arenas lifetime.
 * Nevertheless, freeing blocks is still ok. Free calls MUST be transparent to
 * the caller and behave like the one in a mallocator. But the arena CAN decide
 * itself if it will free anything, or it just ignore the 'free' request (as it
 * is going to free everything soon anyway for example). */
struct Qntan_MemArena {
	/*
	 * See 'Qntan_Mallocator.realloc()'.  */
	void* (*realloc)(
		struct Qntan_MemArena**, void*oldPtr, size_t oldPtr_sz, size_t newPtr_sz );
};










struct Qntan_Executor {
	/*
	 * enque a task to this executor. */
	void (*enque)(
		struct Qntan_Executor**, void(*fn)(Qntan_Cls), Qntan_Cls );
};










/* An IO Multiplexer.
 *
 * WARN: This is considered a low-level module! It SHOULD only be used by
 * low-level modules which abstract away the details of the effective
 * file/socket types in use. High-level modules SHOULD NOT use this type
 * directly.
 *
 * TODO need to clarify what type to use for filedescriptors. nix
 * systems an int would work. But WARN windoof uses HANDLE (aka
 * 'void*'). So maybe 'uintptr_t' to be flexible enough? */
struct Qntan_IoMux {
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/009695399/functions/fread.html".
	 * BUT be aware that there are some differences mentioned here!
	 *
	 * WARN: 'sz' times 'cnt' outside range 0..INT_MAX is UNDEFINED BEHAVIOR! It
	 * is recommended that an implementation fails hard if this happens. For
	 * example by calling 'abort()'.
	 *
	 * @param ptrToFILE
	 *      Is a FILE ptr (NOT FileDescriptor!). In most cases you'll need a
	 *      cast to uintptr_t.
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Zero indicates EOF. Positive
	 *      values indicate number of objects that where read.  */
	void (*read)(
		struct Qntan_IoMux**, void*buf, int sz, int cnt, uintptr_t ptrToFILE,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/009695399/functions/fwrite.html".
	 * BUT be aware that there are some differences mentioned here!
	 *
	 * WARN: 'sz' times 'cnt' outside range 0..INT_MAX is UNDEFINED BEHAVIOR! It
	 * is recommended that an implementation fails hard if this happens. For
	 * example by calling 'abort()'.
	 *
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Positive values indicate
	 *      number of objects written.
	 *      TODO define what zero means.
	 * @param arg
	 *      User defined pointer to pass a context to 'onDone'. */
	void (*write)(
		struct Qntan_IoMux**, void const*buf, int sz, int cnt,
		uintptr_t ptrToFILE, void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/**/
	void (*flush)( struct Qntan_IoMux**, uintptr_t ptrToFILE,
		void(*onDone)(int,Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/000095399/functions/send.html".
	 * BUT be aware that there are some differences mentioned here!
	 * Calls to send MUST NOT overlap. So as long a send call is in progress,
	 * send MUST NOT be called meanwhile.
	 *
	 * @param fd
	 *      FileDescriptor (NOT FILE*!). Or 'HANDLE' on ugly platforms.
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Positive values indicate
	 *      number of objects that where read.
	 * @param arg
	 *      User defined pointer to pass a context to 'onDone'. */
	void (*send)(
		struct Qntan_IoMux**, uintptr_t fd, void const*buf, int len, int flags,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/009695399/functions/recv.html".
	 * BUT be aware that there are some differences mentioned here!
	 * Calls to recv MUST NOT overlap. So as long a recv call is in progress,
	 * recv MUST NOT be called meanwhile.
	 *
	 * @param fd
	 *      FileDescriptor (NOT FILE*!). Or 'HANDLE' on ugly platforms.
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Zero indicates EOF. Positive
	 *      values indicate number of objects that where read.
	 * @param arg
	 *      User defined pointer to pass a context to 'onDone'. */
	void (*recv)(
		struct Qntan_IoMux**, uintptr_t fd, void*buf, int len, int flags,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/009695399/functions/accept.html".
	 * Just with a slightly different API.
	 *
	 * @param sockFd
	 *      FileDescriptor (NOT FILE*!). Or 'HANDLE' on ugly platforms.
	 * @param flgs
	 *      Set 0x1 if you want 'sockaddr' to be filled.
	 *      Setting any other bit is UNDEFINED BEHAVIOR!
	 */
	void (*accept)(
		struct Qntan_IoMux**, uintptr_t sockFd, int flgs,
		void(*onDone)(int ret, uintptr_t newClientFd, void*sockaddr, int sockaddr_len, Qntan_Cls),
		Qntan_Cls );
};










struct Qntan_File {
	/**/
	void (*flush)(
		struct Qntan_File**, void(*onDone)(int ret, Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/000095399/functions/write.html".
	 * BUT be aware that there are some differences mentioned here!
	 * Calls to write MUST NOT overlap. So as long a write call is in progress,
	 * write MUST NOT be called meanwhile.
	 *
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Positive values indicate
	 *      number of objects that where read. */
	void (*write)(
		struct Qntan_File**, void const*buf, int sz, int cnt,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * Basic idea is from
	 * "https://pubs.opengroup.org/onlinepubs/009695399/functions/recv.html".
	 * BUT be aware that there are some differences mentioned here!
	 * Calls to recv MUST NOT overlap. So as long a recv call is in progress,
	 * recv MUST NOT be called meanwhile.
	 *
	 * @param onDone
	 *      Called as soon operation has completed.
	 * @param ret
	 *      Negative numbers indicate errors. Zero indicates EOF. Positive
	 *      values indicate number of objects that where read. */
	void (*read)(
		struct Qntan_File**, void*buf, int sz, int cnt,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * "https://pubs.opengroup.org/onlinepubs/007904875/functions/fseek.html"
	 * TODO: Maybe this func should be without cback? */
	void (*seek)(
		struct Qntan_File**, int64_t off,  int whence,
		void(*onDone)(int ret, Qntan_Cls), Qntan_Cls );
	/*
	 * "https://pubs.opengroup.org/onlinepubs/007904875/functions/ftell.html"
	 * TODO: Maybe this func should be without cback? */
	void (*tell)( struct Qntan_File**,
		void(*onDone)(int64_t ret, Qntan_Cls), Qntan_Cls );
	/*
	 * TODO doc
	 * WARN: size MUST be zero. Any other value is UB! */
	void (*truncate)( struct Qntan_File**, int64_t size,
		void(*onDone)(int,Qntan_Cls), Qntan_Cls );
};










struct Qntan_Process {
	/*
	 * Writes 'buf' to the childs stdin stream.
	 *
	 * @param buf
	 *      This buffer MUST stay valid, until the corresponding
	 *      'onWritten' callback got called!
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!  */
	void (*write)(
		struct Qntan_Process**, void const*buf, int buf_len, int flgs,
		void(*onWritten)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/kill.html
	 *
	 * (HINT: Stay awake to not confuse 'signal' and 'kill' names here)
	 *
	 * Windoof has no SIGKILL. But this API suports it anyway. You can
	 * define it as below in your app to use it:
	 *     #define SIGKILL 132
	 */
	int (*signal)( struct Qntan_Process**, int sig );
	/**/
	void (*join)( struct Qntan_Process**, int timeoutMs,
		void (*onDone)(int,int,int,Qntan_Cls), Qntan_Cls );
};










struct Qntan_HshTbl {
	/*
	 * Returns 0 if 'elm' got copied into the hashtable.
	 * Returns 1 is similar to 'Return 0'. BUT: a matching element did already
	 * exist and this old element now got swapped into 'ptrToDst'.
	 * Returns negative values to indicate errors. */
	int  (*swap)( struct Qntan_HshTbl**, void*elm, void*ptrToDst );
	/*
	 * Returns 0 if 'elm' got copied into the hashtable.
	 * Returns '-EEXIST' (yep, negated) if 'elm' already exists. Nothing got
	 * copied anywhere.
	 * Returns negative values to indicate errors. */
	int  (*addIfNew)( struct Qntan_HshTbl**, void*elm );
	/*
	 * Returns 0 if nothing was removed (eg there's no matching object).
	 * Returns 1 if found object got moved to 'ptrToDst'.
	 * Returns negative values on error. */
	int  (*del)( struct Qntan_HshTbl**, void*srch, void*ptrToDst );
	/*
	 * Returns 0 if nothing was removed (eg there are no elements).
	 * Returns 1 if any object got moved to 'ptrToDst'. It is UNSPECIFIED which
	 * elements gets removed.
	 * Returns negative values on error. */
	int  (*delAny)( struct Qntan_HshTbl**, void*ptrToDst );
	/*
	 * Returns 0 if there's no object matching 'srch'.
	 * Returns 1 if found object got copied to 'ptrToDst'. The element itelf
	 * will swill be present in the table. */
	int  (*get)( struct Qntan_HshTbl**, void*srch, void*ptrToDst );
	/*
	 * Number of objects currently present in this collection. */
	uintptr_t (*nObj)( struct Qntan_HshTbl** );
	/*
	 * Initializes and returns a new cursor. The cursor MUST be passed to
	 * 'delCursor' when no longer used. */
	struct Qntan_HshTbl_Cursor** (*newCursor)( struct Qntan_HshTbl** );
	/**/
	void (*delCursor)( struct Qntan_HshTbl**, struct Qntan_HshTbl_Cursor** );
	/**/
};

struct Qntan_HshTbl_Cursor {
	/*
	 * Returns 1 if next element got placed in 'ptrToDst'.
	 * Returns 0 if there are no more elements to traverse.
	 * Returns negative values on error. */
	int  (*next)( struct Qntan_HshTbl_Cursor**, void*ptrToDst );
	/*
	 * moves the element where cursor currently points to to 'ptrToDst' and
	 * removes it from the table. */
	void (*del)( struct Qntan_HshTbl_Cursor**, void*ptrToDst );
};










struct Qntan_Socket {
	/*
	 * Conceptually same idea as
	 * "https://pubs.opengroup.org/onlinepubs/009696799/functions/accept.html". */
	void (*accept)( struct Qntan_Socket**,
		void(*onDone)(int ret,struct Qntan_Socket**clientSock,Qntan_Cls),
		Qntan_Cls );
	/*
	 * Based on "https://pubs.opengroup.org/onlinepubs/000095399/functions/send.html".
	 *
	 * @param buf, buf_len
	 *      The octets (and how many of them) to write. This 'buf' MUST stay
	 *      valid until onDone got called.
	 *
	 * @param flgs
	 *      (flgs & 1) indicates that data should be flushed.
	 *      (flgs & 2) indicates that after sending buf we have to act like a
	 *      call to
	 *      "https://pubs.opengroup.org/onlinepubs/9699919799/functions/shutdown.html"
	 *      with SHUT_WR.
	 *      All not specified flags MUST be set to zero. Passing any other flag
	 *      is UNDEFINED BEHAVIOR!
	 *
	 * @param onDone
	 *      The function that gets called when the send operation has completed.
	 *      Negative 'ret' indicates errors. 'rbuf' MUST be the same as 'buf'.  */
	void (*send)( struct Qntan_Socket**, void*buf, int buf_len, int flgs,
		void(*onDone)(int ret,void*rbuf,Qntan_Cls), Qntan_Cls );
	/*
	 * Based on "https://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html".
	 *
	 * @param onDone.buf
	 *      MUST be the same ptr as passed via 'buf' initially.  */
	void (*recv)( struct Qntan_Socket**, void*buf, int buf_len,
		void(*onDone)(int ret,void*buf,Qntan_Cls), Qntan_Cls );
	/*
	 * Based on "https://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html".
	 * WARN: Read doc for 'send' carefully before calling 'close'!
	 * BROKEN: Api MUST be re-designed! Pooling doesn't work with TLS this way!
	 */
	void (*close)( struct Qntan_Socket**, void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
};










struct Qntan_TarEnc_Hdr {
	char const*path;  int path_len;
	unsigned mode;
	uint32_t uid, gid;
	char const usrNm[32], grpNm[32];
	uint64_t mTimeEpchSec;
	uint64_t nBodyOctets;
	/**
	 * 0x30=file, 0x31=hardlink, 0x32=symlink. */
	uint8_t linkType;
	char const*link;  int link_len;
};


struct Qntan_TarEnc {
	/*
	 * @param hdr
	 *      The header to begin the next tar entry. From now on, 'write'
	 *      will provide the body belonging to this entry.
	 * @param ret
	 *      Negative values on error.
	 *      Zero on success.
	 *      Any other values are UNDEFINED BEHAVIOR!  */
	void (*nextEntry)( struct Qntan_TarEnc**, struct Qntan_TarEnc_Hdr*hdr,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * @param buf
	 *      The next content chunk which belongs to the most recently
	 *      written header.
	 * @param buf_len
	 *      How many octets to write from 'buf'.
	 * @param flgs
	 *      0x4 set means this is last chunk of this entry.
	 *      0x8 set means, that the whole archive has completed.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 * @param ret
	 *      Negative on errors.
	 *      Count of octets successfully written.  */
	void (*write)( struct Qntan_TarEnc**, void*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * Returns the most recent error as a short string message. The
	 * buffer onwership stays within the impl. So take a copy if you
	 * need it later. */
	char const* (*getLastErrorStr)( struct Qntan_TarEnc** );
};
















struct Qntan_TarDec_Hdr {
	char const*path;  int path_len;
	unsigned mode;
	uint32_t uid, gid;
	char const *usrNm;  int usrNm_len;
	char const *grpNm;  int grpNm_len;
	uint64_t mTimeEpchSec;
	uint64_t nBodyOctets;
	/**
	 * 0x30=file, 0x31=hardlink, 0x32=symlink. */
	uint8_t linkType;
	char const*link;  int link_len;
	uint32_t checksum;
	/* TODO doc */
	uint8_t filetype;
};

struct Qntan_TarDec {
	/*
	 * @param ret
	 *      Negative values on error.
	 *      1 (one) if 'hdr' now contains the requested header.
	 *      0 (zero) if there are no more entries (EndOf archive reached).
	 * @param hdr
	 *      Iff 'ret' is one, this contains the requested header.
	 *      WARN: 'hdr' (and stuff where it points to) is only valid uppon
	 *            return from 'onDone'.
	 *      WARN: for any other value of 'ret', value of 'hdr' is unspecified!
	 */
	void (*nextHdr)( struct Qntan_TarDec**,
		void(*onDone)(int ret, struct Qntan_TarDec_Hdr*hdr,Qntan_Cls), Qntan_Cls );
	/*
	 * @param buf
	 *      Where to write the read octets to.
	 * @param buf_cap
	 *      Maximum count of octets to read.
	 * @param rbuf
	 *      Same as 'buf', just for convenience to have a reference in
	 *      the callback also.
	 * @param buf_len
	 *      How many octets actually got read and placed into 'buf'.
	 * @param flg
	 *      'flg & 4' indicates that there is no more data (EndOf entry
	 *      reached).
	 */
	void (*readBody)( struct Qntan_TarDec**, void*buf, int buf_cap,
		void(*onDone)(void*rbuf, int buf_len, int flg, Qntan_Cls), Qntan_Cls);
};










struct Qntan_Networker {
	/*
	 * Inspired by
	 * "https://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html"
	 * but tweaks the api a bit toward callbacks, so it fits better into the idea
	 * of this libraries asynchronous nature.
	 *
	 * @param onAddress
	 *     MUST either return zero to indicate success or -ECANCELED (negated)
	 *     if iteration should stop.
	 *     Returning any other value is UNDEFINED BEHAVIOR!
	 * @param onDone
	 *     called at end of operation.  */
	void (*getaddrinfo)(
		struct Qntan_Networker**, char const*node, int node_len,
		char const*service, int service_len,
		int(*onAddress)(int ai_flags, int ai_family, int ai_socktype, int ai_protocol,
			void*sockaddr, int sockaddr_len, char const*ai_canonname, Qntan_Cls),
		void(*onDone)(int ret, char const*errmsg, Qntan_Cls),
		Qntan_Cls );
};










struct Qntan_CsvDec {
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)( struct Qntan_CsvDec**,
		void const*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
};


struct Qntan_CsvDec_Mentor {
	/*
	 * TODO doc */
	void (*onCsvDocEnd)( Qntan_Cls );
	/*
	 * TODO doc */
	void (*onEntityBeg)( Qntan_Cls );
	/*
	 * TODO doc */
	void (*onEntityEnd)( Qntan_Cls );
	/*
	 * TODO doc */
	void (*onAttrBeg)( Qntan_Cls );
	/*
	 * TODO doc */
	void (*onAttrEnd)( Qntan_Cls );
	/*
	 * TODO doc */
	void (*onChunkNaked)( char const*buf, int buf_len, Qntan_Cls );
	/*
	 * TODO doc */
	void (*onChunkQuoted)( char const*buf, int buf_len, Qntan_Cls );
};










struct Qntan_CsvIStream {
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)( struct Qntan_CsvIStream**,
		void const*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
	/*
	 * TODO doc */
	void (*pause)( struct Qntan_CsvIStream** );
	/*
	 * TODO doc */
	void (*resume)( struct Qntan_CsvIStream** );
};

struct Qntan_CsvIStream_Mentor {
	/*
	 * Called for every CSV row.
	 *
	 * How an implementation could look like:
	 *   void onCsvRow(char const*values, int*values_len, int numCols, Qntan_Cls a){
	 *       for( int iCol = 0 ; iCol < numCols ; ++iCol )
	 *           printf("col[%d]: '%.*s'\n", iCol, values_len[i], values[i]);
	 *   }
	 *
	 * @param values
	 *      Array of strings of which this row consists of.
	 * @param values_len
	 *      Lengths of the strings in 'values'.
	 * @param numCols
	 *      Number of columns present in this row. Aka number of entries present
	 *      in 'values' and 'values_len'.  */
	void (*onCsvRow)(
		char const**values, int*values_len, int numCols, Qntan_Cls );
	/*
	 * Called when there are no more rows. */
	void (*onCsvDocEnd)( Qntan_Cls );
};










struct Qntan_JsonEnc {
	/*
	 * throws away the current state and sets the encoder for a new run. */
	void (*reset)( struct Qntan_JsonEnc** );
	/**/
	void (*flush)( struct Qntan_JsonEnc** );
	/**/
	void (*objBeg)( struct Qntan_JsonEnc** );
	/**/
	void (*objEnd)( struct Qntan_JsonEnc** );
	/**/
	void (*arrBeg)( struct Qntan_JsonEnc** );
	/**/
	void (*arrEnd)( struct Qntan_JsonEnc** );
	/*
	 * Can be called multiple times, which will append to the currently in
	 * progress key. The last call MUST have flg 0x4 set, to indicate end.
	 * 'key' MUST be in utf8 encoding!
	 * Any other flags set leads to UNDEFINED BEHAVIOR! */
	void (*key)( struct Qntan_JsonEnc**, char const*key, int key_len, int flgs );
	/*
	 * Can be called multiple times, which will append to the currently in
	 * progress string. The last call MUST have flg 0x4 set, to indicate end.
	 * 'str' MUST be in utf8 encoding!
	 * Any other flags set leads to UNDEFINED BEHAVIOR! */
	void (*string)( struct Qntan_JsonEnc**, char const*str, int str_len, int flgs );
	/**/
	void (*number)( struct Qntan_JsonEnc**, double );
	/**/
	void (*boolean)( struct Qntan_JsonEnc**, int );
	/**/
	void (*null)( struct Qntan_JsonEnc** );
	/**/
};










struct Qntan_JsonDec {
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)(struct Qntan_JsonDec**, void const*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
};


struct Qntan_JsonDec_Mentor {
    /*
     * Called if a parsing error occurs. 'errOff' is a byte offset measured from
     * the beginning of the json to the char where the error was detected. Be
     * prepared that the error may not be exactly at the pointed location, but
     * somewhere near that location. */
    void (*onError)( Qntan_Cls cls, uintmax_t errOff );
    /**/
    void (*onJsonDocEnd)( Qntan_Cls cls );
    /**/
    void (*onJsonObjBeg)( Qntan_Cls cls );
    /**/
    void (*onJsonObjEnd)( Qntan_Cls cls );
    /**/
    void (*onJsonArrBeg)( Qntan_Cls cls );
    /**/
    void (*onJsonArrEnd)( Qntan_Cls cls );
    /**/
    void (*onJsonNull)( Qntan_Cls cls );
    /**/
    void (*onJsonBool)( int val, Qntan_Cls cls );
    /**/
    void (*onJsonNumber)( double value, Qntan_Cls cls );
    /*
     * zero-length chunk indicates EOF */
    void (*onJsonStringKeyChunk)( char const*key, int key_len, Qntan_Cls cls );
    /*
     * zero-length chunk indicates EOF */
    void (*onJsonStringChunk)( char const*buf, int buf_len, Qntan_Cls cls );
};










struct Qntan_JsonTreeDec_JsonNode {
    /*
     * '{'  object
     * '['  array
     * 'f'  float (aka number)
     * 'b'  bool
     * 's'  string
     * '0'  null  */
    char type;
    /*
     * Is NULL for the root node. */
    struct Qntan_JsonTreeDec_JsonNode *parent;
    /*
     * If this node is a child in an object, its key will be stored here. So
     * for example in JSON '{"foo":42}' if we take current node to be
     * number 42, the 'key' will contain "foo". 'key_len' is number of BYTES,
     * NOT codepoints. */
    char *key;  int key_len;
    /*
     * See 'type' to know which of those values is valid. */
    union {
        double valNumber;
        int valBool;
        struct/*valString*/{
            char const *valStr;
            int valStr_len;
        };
        struct/*object or array. For object keys see 'childs[i]->key' */{
            struct Qntan_JsonTreeDec_JsonNode **childs;
            int childs_len;
        };
    };
};

struct Qntan_JsonTreeDec {
    /*
     * flg bit 0x4 means "isLastChunk". */
    void (*write)( struct Qntan_JsonTreeDec**, void const*buf, int buf_len, int flg,
        void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
    /**/
};










struct Qntan_XmlDec {
	/**/
	void (*pause)( struct Qntan_XmlDec** );
	/**/
	void (*resume)( struct Qntan_XmlDec** );
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)( struct Qntan_XmlDec**,
		void const*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
};


struct Qntan_XmlDec_Mentor {
	/**/
	void (*onXmlDocEnd)( Qntan_Cls );
	/**/
	void (*onElemBeg)( char const*tagName, int tagName_len, Qntan_Cls );
	/**/
	void (*onElemEnd)( char const*tagName, int tagName_len, Qntan_Cls );
	/**/
	void (*onAttr)( char const*key, char const*val, Qntan_Cls );
	/**/
	void (*onChunk)( char const*buf, int buf_len, Qntan_Cls );
	/**/
};










struct Qntan_JavaBytecodeParser {
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)(
		struct Qntan_JavaBytecodeParser**, char const*buf, int buf_len, int flgs,
		void(*onDone)(int ret,Qntan_Cls), Qntan_Cls );
};

/**
 * WARN: Be aware that ConstPool indexes are ONE based as by the java
 *       specification.
 */
struct Qntan_JavaBytecodeParser_Mentor {
	/**/
	void (*onMagic)( uint32_t magic, struct Qntan_JavaBytecodeParser_Mentor** );
	/**/
	void (*onClassfileVersion)(
		int major, int minor, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolBegin)( int poolSize, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolUtf8)( int poolNr, char const*buf, int buf_len,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolInteger)( int poolNr, int32_t intValue,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolFloat)( int poolNr, float value,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolLong)( int poolNr, int64_t value,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolDouble)( int poolNr, double value,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolStrRef)( int poolNr, int dstIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolClassRef)( int poolNr, int nameIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolMethodRef)( int poolNr, int classIdx, int nameAndTypeIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolIfaceMethodRef)( int poolNr, int nameIdx, int nameAndTypeIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolFieldRef)( int poolNr, int nameIdx, int nameAndTypeIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolNameAndType)( int poolNr, int nameIdx, int typeIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolMethodHandle)( int poolNr, int refKind, int refIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolMethodType)( int poolNr, int descrIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolInvokeDynamic)( int poolNr, int bootstrapMethodAttrIdx, int nameAndTypeIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolModule)( int poolNr, uint16_t nameIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolPackage)( int poolNr, uint16_t nameIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onConstPoolEnd)( struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onAccessFlags)( uint16_t accessFlags, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onThisClass)( int nameIdx, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onSuperClass)( int nameIdx, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onIfacesBegin)( int numIfaces, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onIface)( int iIface, int classIdx, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/*
	 * TODO is this cback really needed?*/
	void (*onIfacesEnd)( struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onFieldsBegin)( uint16_t numFields,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onField)( uint16_t poolIdx, uint16_t accessFlags, uint16_t nameIdx,
		uint16_t descrIdx, uint16_t numAttrs,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onMethodsBegin)( int numMethods, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onMethod)( uint16_t accessFlags, int nameIdx, int descrIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onClassAttrsBegin)( int numAttrs, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onAnnotRtVisible)( uint16_t typeIdx, uint16_t numKeyValPairs,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onAnnotRtInVisible)( uint16_t typeIdx, uint16_t numKeyValPairs,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onAnnotKeyValPair)( uint8_t type, uint16_t typeNameIdx,
		uint16_t constNameIdx, struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onAnnotKey)( uint8_t type, uint16_t typeNameIdx,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onGenericAttribute)( int nameIdx, uint32_t bodyLength,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onGenericAttributeContent)( uint8_t const*buf, int buf_len,
		struct Qntan_JavaBytecodeParser_Mentor**cls );
	/**/
	void (*onEnd)( struct Qntan_JavaBytecodeParser_Mentor**cls );
};










struct Qntan_CamtAccount {
	char iban[32];
	char ccy[4];
};

struct Qntan_CamtTransaction {
	int valueDate_cap, valueDate_len; char *valueDate;
	int bookDate_cap , bookDate_len ; char *bookDate ;
	char isCrdt, isDbit;
	char ccy[4];
	int amtDst_cap   , amtDst_len   ; char *amtDst   ;
	int amtSrc_cap   , amtSrc_len   ; char *amtSrc   ;
	int dstIban_cap  , dstIban_len  ; char *dstIban  ;
	int srcIban_cap  , srcIban_len  ; char *srcIban  ;
	int ustrd_cap    , ustrd_len    ; char *ustrd    ;
	int addtlInf_cap , addtlInf_len ; char *addtlInf ;
	int qrRef_cap    , qrRef_len    ; char *qrRef    ;
};

struct Qntan_CamtDec_Mentor {
	/**/
	void (*onAccount)( struct Qntan_CamtDec_Mentor**,
		struct Qntan_CamtAccount*, void(*onDone)(Qntan_Cls), Qntan_Cls );
	/**/
	void (*onRecord)( struct Qntan_CamtDec_Mentor**,
		struct Qntan_CamtTransaction*, void(*onDone)(Qntan_Cls), Qntan_Cls );
};

struct Qntan_CamtDec {
	/*
	 * @param flgs
	 *      0x4 set means this is last chunk.
	 *      Any other bit set is UNDEFINED BEHAVIOR!
	 */
	void (*write)(
		struct Qntan_CamtDec**, void*buf, int buf_len, int flgs,
		void(*onDone)(int,Qntan_Cls), Qntan_Cls );
};










#if __cplusplus
} /* extern "C" */
#endif
#endif /* INCGUARD_aM91R0ljMWoqtn7F */
