/*------------------------------------------------------------------------*/
/* A Sample Code of User Provided OS Dependent Functions for FatFs        */
/*------------------------------------------------------------------------*/

#include "ff.h"

#if FF_FS_NORTC == 0

DWORD get_fattime() {
	return (DWORD)0 << 25 |
		   (DWORD)1 << 21 |
		   (DWORD)1 << 16 |
		   (DWORD)0 << 11 |
		   (DWORD)0 << 5 |
		   (DWORD)0;
}

#endif /* FF_FS_NORTC == 0 */

#if FF_USE_LFN == 3	/* Use dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate/Free a Memory Block                                           */
/*------------------------------------------------------------------------*/

#define LFN_BUFFERS	5
static uint8_t lfn_buffers[LFN_BUFFERS][(FF_MAX_LFN + 1) * 2];
static uint8_t lfn_buffer_used[LFN_BUFFERS] = {0, 0, 0, 0, 0};


void* ff_memalloc (	/* Returns pointer to the allocated memory block (null if not enough core) */
	UINT msize		/* Number of bytes to allocate */
)
{
	if (msize != (FF_MAX_LFN + 1) * 2) {
		return 0;
	}
	for (uint8_t i = 0; i < LFN_BUFFERS; i++) {
		if (lfn_buffer_used[i] == 0) {
			lfn_buffer_used[i] = 1;
			return lfn_buffers[i];
		}
	}
	return 0;
}


void ff_memfree (
	void* mblock	/* Pointer to the memory block to free (no effect if null) */
)
{
	for (uint8_t i = 0; i < LFN_BUFFERS; i++) {
		if (lfn_buffers[i] == mblock) {
			lfn_buffer_used[i] = 0;
			return;
		}
	}
}

#endif

#if FF_FS_REENTRANT	/* Mutal exclusion */
/*------------------------------------------------------------------------*/
/* Definitions of Mutex                                                   */
/*------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"
static StaticSemaphore_t Mutex_ctrl[FF_VOLUMES + 1];
static SemaphoreHandle_t Mutex[FF_VOLUMES + 1];	/* Table of mutex handle */


/*------------------------------------------------------------------------*/
/* Create a Mutex                                                         */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount function to create a new mutex
/  or semaphore for the volume. When a 0 is returned, the f_mount function
/  fails with FR_INT_ERR.
*/

int ff_mutex_create (	/* Returns 1:Function succeeded or 0:Could not create the mutex */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	Mutex[vol] = xSemaphoreCreateMutexStatic(&Mutex_ctrl[vol]);
	return 1;
}


/*------------------------------------------------------------------------*/
/* Delete a Mutex                                                         */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount function to delete a mutex or
/  semaphore of the volume created with ff_mutex_create function.
*/

void ff_mutex_delete (	/* Returns 1:Function succeeded or 0:Could not delete due to an error */
	int vol				/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	vSemaphoreDelete(Mutex[vol]);
}


/*------------------------------------------------------------------------*/
/* Request a Grant to Access the Volume                                   */
/*------------------------------------------------------------------------*/
/* This function is called on enter file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_mutex_take (	/* Returns 1:Succeeded or 0:Timeout */
	int vol			/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	return xSemaphoreTake(Mutex[vol], pdMS_TO_TICKS(FF_FS_TIMEOUT)) == pdTRUE ? 1 : 0;
}



/*------------------------------------------------------------------------*/
/* Release a Grant to Access the Volume                                   */
/*------------------------------------------------------------------------*/
/* This function is called on leave file functions to unlock the volume.
*/

void ff_mutex_give (
	int vol			/* Mutex ID: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES) */
)
{
	xSemaphoreGive(Mutex[vol]);
}

#endif	/* FF_FS_REENTRANT */

