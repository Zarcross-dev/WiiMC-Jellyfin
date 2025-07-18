/*****************************************************************************
 * device.h: DVD device access
 *****************************************************************************
 * Copyright (C) 1998-2006 VideoLAN
 * $Id: device.c 31156 2010-05-11 10:58:50Z diego $
 *
 * Authors: Stéphane Borel <stef@via.ecp.fr>
 *          Sam Hocevar <sam@zoy.org>
 *          Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_ERRNO_H
#   include <errno.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#endif
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#   include <limits.h>
#endif

#if defined( WIN32 ) && !defined( SYS_CYGWIN )
#   include <io.h>                                                 /* read() */
#elif !defined ( GEKKO )
#   include <sys/uio.h>                                      /* struct iovec */
#endif

#ifdef GEKKO
#    include <ogc/usbstorage.h>
#    include <di/di.h>
#    include <malloc.h>
#    include "../../utils/mem2_manager.h"
extern bool StartDVDMotor();
#endif

#ifdef DARWIN_DVD_IOCTL
#   include <paths.h>
#   include <CoreFoundation/CoreFoundation.h>
#   include <IOKit/IOKitLib.h>
#   include <IOKit/IOBSD.h>
#   include <IOKit/storage/IOMedia.h>
#   include <IOKit/storage/IOCDMedia.h>
#   include <IOKit/storage/IODVDMedia.h>
#endif

#ifdef SYS_OS2
#   define INCL_DOS
#   define INCL_DOSDEVIOCTL
#   include <os2.h>
#   include <io.h>                                              /* setmode() */
#   include <fcntl.h>                                           /* O_BINARY  */
#endif

#include "dvdcss/dvdcss.h"

#include "common.h"
#include "css.h"
#include "libdvdcss.h"
#include "ioctl.h"
#include "device.h"

/*****************************************************************************
 * Device reading prototypes
 *****************************************************************************/
static int libc_open  ( dvdcss_t, char const * );
static int libc_seek  ( dvdcss_t, int );
static int libc_read  ( dvdcss_t, void *, int );
static int libc_readv ( dvdcss_t, struct iovec *, int );

#ifdef GEKKO
static int di_open  ( dvdcss_t, char const * );
static int di_seek  ( dvdcss_t, int );
static int di_read  ( dvdcss_t, void *, int );
static int di_readv ( dvdcss_t, struct iovec *, int );

static int usb_open  ( dvdcss_t, char const * );
static int usb_seek  ( dvdcss_t, int );
static int usb_read  ( dvdcss_t, void *, int );
static int usb_readv ( dvdcss_t, struct iovec *, int );
static const DISC_INTERFACE *usb = &__io_usbstorage;
#endif

#ifdef WIN32
static int win2k_open ( dvdcss_t, char const * );
static int aspi_open  ( dvdcss_t, char const * );
static int win2k_seek ( dvdcss_t, int );
static int aspi_seek  ( dvdcss_t, int );
static int win2k_read ( dvdcss_t, void *, int );
static int aspi_read  ( dvdcss_t, void *, int );
static int win_readv  ( dvdcss_t, struct iovec *, int );

static int aspi_read_internal  ( int, void *, int );
#elif defined( SYS_OS2 )
static int os2_open ( dvdcss_t, char const * );
/* just use macros for libc */
#   define os2_seek     libc_seek
#   define os2_read     libc_read
#   define os2_readv    libc_readv
#endif

int _dvdcss_use_ioctls( dvdcss_t dvdcss )
{
#if defined( WIN32 )
    if( dvdcss->b_file )
    {
        return 0;
    }

    /* FIXME: implement this for Windows */
    if( WIN2K )
    {
        return 1;
    }
    else
    {
        return 1;
    }
#elif defined( SYS_OS2 )
    ULONG ulMode;

    if( DosQueryFHState( dvdcss->i_fd, &ulMode ) != 0 )
        return 1;  /* What to do?  Be conservative and try to use the ioctls */

    if( ulMode & OPEN_FLAGS_DASD )
        return 1;

    return 0;
#elif defined( GEKKO )
    if (dvdcss->pf_read == usb_read)
        return 1;
    else if (dvdcss->pf_read == di_read)
        return 0;
    else
    return 0;
#else
    struct stat fileinfo;
    int ret;

    ret = fstat( dvdcss->i_fd, &fileinfo );
    if( ret < 0 )
    {
        return 1;  /* What to do?  Be conservative and try to use the ioctls */
    }

    /* Complete this list and check that we test for the right things
     * (I've assumed for all OSs that 'r', (raw) device, are char devices
     *  and those that don't contain/use an 'r' in the name are block devices)
     *
     * Linux    needs a block device
     * Solaris  needs a char device
     * Darwin   needs a char device
     * OpenBSD  needs a char device
     * NetBSD   needs a char device
     * FreeBSD  can use either the block or the char device
     * BSD/OS   can use either the block or the char device
     */

    /* Check if this is a block/char device */
    if( S_ISBLK( fileinfo.st_mode ) ||
        S_ISCHR( fileinfo.st_mode ) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

void _dvdcss_check ( dvdcss_t dvdcss )
{
#if defined( WIN32 )
    DWORD drives;
    int i;
#elif defined( DARWIN_DVD_IOCTL )
    io_object_t next_media;
    mach_port_t master_port;
    kern_return_t kern_result;
    io_iterator_t media_iterator;
    CFMutableDictionaryRef classes_to_match;
#elif defined( SYS_OS2 )
#pragma pack( 1 )
    struct
    {
        BYTE bCmdInfo;
        BYTE bDrive;
    } param;

    struct
    {
        BYTE    abEBPB[31];
        USHORT  usCylinders;
        BYTE    bDevType;
        USHORT  usDevAttr;
    } data;
#pragma pack()

    ULONG ulParamLen;
    ULONG ulDataLen;
    ULONG rc;

    int i;
#elif defined( GEKKO )

#else
    char *ppsz_devices[] = { "/dev/dvd", "/dev/cdrom", "/dev/hdc", NULL };
    int i, i_fd;
#endif

    /* If the device name is non-null, return */
    if( dvdcss->psz_device[0] )
    {
        return;
    }

#if defined( WIN32 )
    drives = GetLogicalDrives();

    for( i = 0; drives; i++ )
    {
        char psz_device[5];
        DWORD cur = 1 << i;
        UINT i_ret;

        if( (drives & cur) == 0 )
        {
            continue;
        }
        drives &= ~cur;

        sprintf( psz_device, "%c:\\", 'A' + i );
        i_ret = GetDriveType( psz_device );
        if( i_ret != DRIVE_CDROM )
        {
            continue;
        }

        /* Remove trailing backslash */
        psz_device[2] = '\0';

        /* FIXME: we want to differenciate between CD and DVD drives
         * using DeviceIoControl() */
        print_debug( dvdcss, "defaulting to drive `%s'", psz_device );
        free( dvdcss->psz_device );
        dvdcss->psz_device = strdup( psz_device );
        return;
    }
#elif defined( DARWIN_DVD_IOCTL )

    kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    classes_to_match = IOServiceMatching( kIODVDMediaClass );
    if( classes_to_match == NULL )
    {
        return;
    }

    CFDictionarySetValue( classes_to_match, CFSTR( kIOMediaEjectableKey ),
                          kCFBooleanTrue );

    kern_result = IOServiceGetMatchingServices( master_port, classes_to_match,
                                                &media_iterator );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    next_media = IOIteratorNext( media_iterator );
    for( ; ; )
    {
        char psz_buf[0x32];
        size_t i_pathlen;
        CFTypeRef psz_path;

        next_media = IOIteratorNext( media_iterator );
        if( next_media == 0 )
        {
            break;
        }

        psz_path = IORegistryEntryCreateCFProperty( next_media,
                                                    CFSTR( kIOBSDNameKey ),
                                                    kCFAllocatorDefault,
                                                    0 );
        if( psz_path == NULL )
        {
            IOObjectRelease( next_media );
            continue;
        }

        snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
        i_pathlen = strlen( psz_buf );

        if( CFStringGetCString( psz_path,
                                (char*)&psz_buf + i_pathlen,
                                sizeof(psz_buf) - i_pathlen,
                                kCFStringEncodingASCII ) )
        {
            print_debug( dvdcss, "defaulting to drive `%s'", psz_buf );
            CFRelease( psz_path );
            IOObjectRelease( next_media );
            IOObjectRelease( media_iterator );
            free( dvdcss->psz_device );
            dvdcss->psz_device = strdup( psz_buf );
            return;
        }

        CFRelease( psz_path );

        IOObjectRelease( next_media );
    }

    IOObjectRelease( media_iterator );
#elif defined( GEKKO )
    free( dvdcss->psz_device );
    dvdcss->psz_device = strdup("/dev/di");
    print_debug( dvdcss, "defaulting to drive `%s'", dvdcss->psz_device );
#elif defined( SYS_OS2 )
    for( i = 0; i < 26; i++ )
    {
        param.bCmdInfo = 0;
        param.bDrive = i;

        rc = DosDevIOCtl( ( HFILE )-1, IOCTL_DISK, DSK_GETDEVICEPARAMS,
                          &param, sizeof( param ), &ulParamLen,
                          &data, sizeof( data ), &ulDataLen );

        if( rc == 0 )
        {
            /* Check for removable and for cylinders */
            if( ( data.usDevAttr & 1 ) == 0 && data.usCylinders == 0xFFFF )
            {
                char psz_dvd[] = "A:";

                psz_dvd[0] += i;

                print_debug( dvdcss, "defaulting to drive `%s'", psz_dvd );
                free( dvdcss->psz_device );
                dvdcss->psz_device = strdup( psz_dvd );
                return;
            }
        }
    }
#else
    for( i = 0; ppsz_devices[i]; i++ )
    {
        i_fd = open( ppsz_devices[i], 0 );
        if( i_fd != -1 )
        {
            print_debug( dvdcss, "defaulting to drive `%s'", ppsz_devices[i] );
            close( i_fd );
            free( dvdcss->psz_device );
            dvdcss->psz_device = strdup( ppsz_devices[i] );
            return;
        }
    }
#endif

    print_error( dvdcss, "could not find a suitable default drive" );
}

int _dvdcss_open ( dvdcss_t dvdcss )
{
    char const *psz_device = dvdcss->psz_device;

    print_debug( dvdcss, "opening target `%s'", psz_device );

#if defined( WIN32 )
    dvdcss->b_file = 1;
    /* If device is "X:" or "X:\", we are not actually opening a file. */
    if (psz_device[0] && psz_device[1] == ':' &&
       (!psz_device[2] || (psz_device[2] == '\\' && !psz_device[3])))
        dvdcss->b_file = 0;

    /* Initialize readv temporary buffer */
    dvdcss->p_readv_buffer   = NULL;
    dvdcss->i_readv_buf_size = 0;

    if( !dvdcss->b_file && WIN2K )
    {
        print_debug( dvdcss, "using Win2K API for access" );
        dvdcss->pf_seek  = win2k_seek;
        dvdcss->pf_read  = win2k_read;
        dvdcss->pf_readv = win_readv;
        return win2k_open( dvdcss, psz_device );
    }
    else if( !dvdcss->b_file )
    {
        print_debug( dvdcss, "using ASPI for access" );
        dvdcss->pf_seek  = aspi_seek;
        dvdcss->pf_read  = aspi_read;
        dvdcss->pf_readv = win_readv;
        return aspi_open( dvdcss, psz_device );
    }
    else
#elif defined( SYS_OS2 )
    /* If device is "X:" or "X:\", we are not actually opening a file. */
    if( psz_device[0] && psz_device[1] == ':' &&
        ( !psz_device[2] || ( psz_device[2] == '\\' && !psz_device[3] ) ) )
    {
        print_debug( dvdcss, "using OS2 API for access" );
        dvdcss->pf_seek  = os2_seek;
        dvdcss->pf_read  = os2_read;
        dvdcss->pf_readv = os2_readv;
        return os2_open( dvdcss, psz_device );
    }
    else
#elif defined( GEKKO )
    if ( !strcmp(psz_device, "/dev/di") )
    {
        print_debug( dvdcss, "using Erant's DI API for access" );
        dvdcss->pf_seek  = di_seek;
        dvdcss->pf_read  = di_read;
        dvdcss->pf_readv = di_readv;
        return di_open( dvdcss, psz_device );
    }
    else if ( !strcmp(psz_device, "/dev/usb") )
    {
        print_debug( dvdcss, "using Wii USB API for access" );
        dvdcss->pf_seek  = usb_seek;
        dvdcss->pf_read  = usb_read;
        dvdcss->pf_readv = usb_readv;
        return usb_open( dvdcss, psz_device );
    }
    else
#endif
    {
        print_debug( dvdcss, "using libc for access" );
        dvdcss->pf_seek  = libc_seek;
        dvdcss->pf_read  = libc_read;
        dvdcss->pf_readv = libc_readv;
        return libc_open( dvdcss, psz_device );
    }
}

#if !defined(WIN32) && !defined(SYS_OS2)
int _dvdcss_raw_open ( dvdcss_t dvdcss, char const *psz_device )
{
    dvdcss->i_raw_fd = open( psz_device, 0 );

    if( dvdcss->i_raw_fd == -1 )
    {
        print_debug( dvdcss, "cannot open %s (%s)",
                             psz_device, strerror(errno) );
        print_error( dvdcss, "failed to open raw device, but continuing" );
        return -1;
    }
    else
    {
        dvdcss->i_read_fd = dvdcss->i_raw_fd;
    }

    return 0;
}
#endif

int _dvdcss_close ( dvdcss_t dvdcss )
{
#if defined( WIN32 )
    if( dvdcss->b_file )
    {
        close( dvdcss->i_fd );
    }
    else if( WIN2K )
    {
        CloseHandle( (HANDLE) dvdcss->i_fd );
    }
    else /* ASPI */
    {
        struct w32_aspidev *fd = (struct w32_aspidev *) dvdcss->i_fd;

        /* Unload aspi and free w32_aspidev structure */
        FreeLibrary( (HMODULE) fd->hASPI );
        free( (void*) dvdcss->i_fd );
    }

    /* Free readv temporary buffer */
    if( dvdcss->p_readv_buffer )
    {
        free( dvdcss->p_readv_buffer );
        dvdcss->p_readv_buffer   = NULL;
        dvdcss->i_readv_buf_size = 0;
    }

    return 0;
#else
    close( dvdcss->i_fd );

#ifndef SYS_OS2
    if( dvdcss->i_raw_fd >= 0 )
    {
        close( dvdcss->i_raw_fd );
        dvdcss->i_raw_fd = -1;
    }
#endif

    return 0;
#endif
}

/* Following functions are local */

/*****************************************************************************
 * Open commands.
 *****************************************************************************/
static int libc_open ( dvdcss_t dvdcss, char const *psz_device )
{
#if !defined( WIN32 ) && !defined( SYS_OS2 )
    dvdcss->i_fd = dvdcss->i_read_fd = open( psz_device, 0 );
#else
    dvdcss->i_fd = dvdcss->i_read_fd = open( psz_device, O_BINARY );
#endif

    if( dvdcss->i_fd == -1 )
    {
        print_debug( dvdcss, "cannot open %s (%s)",
                             psz_device, strerror(errno) );
        print_error( dvdcss, "failed to open device" );
        return -1;
    }

    dvdcss->i_pos = 0;

    return 0;
}

#if defined( WIN32 )
static int win2k_open ( dvdcss_t dvdcss, char const *psz_device )
{
    char psz_dvd[7];
    _snprintf( psz_dvd, 7, "\\\\.\\%c:", psz_device[0] );

    /* To work around an M$ bug in IOCTL_DVD_READ_STRUCTURE, we need read
     * _and_ write access to the device (so we can make SCSI Pass Through
     * Requests). Unfortunately this is only allowed if you have
     * administrator priviledges so we allow for a fallback method with
     * only read access to the device (in this case ioctl_ReadCopyright()
     * won't send back the right result).
     * (See Microsoft Q241374: Read and Write Access Required for SCSI
     * Pass Through Requests) */
    dvdcss->i_fd = (int)
                CreateFile( psz_dvd, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_RANDOM_ACCESS, NULL );

    if( (HANDLE) dvdcss->i_fd == INVALID_HANDLE_VALUE )
        dvdcss->i_fd = (int)
                    CreateFile( psz_dvd, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING,
                                FILE_FLAG_RANDOM_ACCESS, NULL );

    if( (HANDLE) dvdcss->i_fd == INVALID_HANDLE_VALUE )
    {
        print_error( dvdcss, "failed opening device" );
        return -1;
    }

    dvdcss->i_pos = 0;

    return 0;
}

static int aspi_open( dvdcss_t dvdcss, char const * psz_device )
{
    HMODULE hASPI;
    DWORD dwSupportInfo;
    struct w32_aspidev *fd;
    int i, j, i_hostadapters;
    GETASPI32SUPPORTINFO lpGetSupport;
    SENDASPI32COMMAND lpSendCommand;
    char c_drive = psz_device[0];

    /* load aspi and init w32_aspidev structure */
    hASPI = LoadLibrary( "wnaspi32.dll" );
    if( hASPI == NULL )
    {
        print_error( dvdcss, "unable to load wnaspi32.dll" );
        return -1;
    }

    lpGetSupport = (GETASPI32SUPPORTINFO) GetProcAddress( hASPI, "GetASPI32SupportInfo" );
    lpSendCommand = (SENDASPI32COMMAND) GetProcAddress( hASPI, "SendASPI32Command" );

    if(lpGetSupport == NULL || lpSendCommand == NULL )
    {
        print_error( dvdcss, "unable to get aspi function pointers" );
        FreeLibrary( hASPI );
        return -1;
    }

    dwSupportInfo = lpGetSupport();

    if( HIBYTE( LOWORD ( dwSupportInfo ) ) == SS_NO_ADAPTERS )
    {
        print_error( dvdcss, "no ASPI adapters found" );
        FreeLibrary( hASPI );
        return -1;
    }

    if( HIBYTE( LOWORD ( dwSupportInfo ) ) != SS_COMP )
    {
        print_error( dvdcss, "unable to initalize aspi layer" );
        FreeLibrary( hASPI );
        return -1;
    }

    i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
    if( i_hostadapters == 0 )
    {
        print_error( dvdcss, "no ASPI adapters ready" );
        FreeLibrary( hASPI );
        return -1;
    }

    fd = malloc( sizeof( struct w32_aspidev ) );
    if( fd == NULL )
    {
        print_error( dvdcss, "not enough memory" );
        FreeLibrary( hASPI );
        return -1;
    }

    fd->i_blocks = 0;
    fd->hASPI = (long) hASPI;
    fd->lpSendCommand = lpSendCommand;

    c_drive = c_drive > 'Z' ? c_drive - 'a' : c_drive - 'A';

    for( i = 0; i < i_hostadapters; i++ )
    {
        for( j = 0; j < 15; j++ )
        {
            struct SRB_GetDiskInfo srbDiskInfo;

            srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
            srbDiskInfo.SRB_HaId        = i;
            srbDiskInfo.SRB_Flags       = 0;
            srbDiskInfo.SRB_Hdr_Rsvd    = 0;
            srbDiskInfo.SRB_Target      = j;
            srbDiskInfo.SRB_Lun         = 0;

            lpSendCommand( (void*) &srbDiskInfo );

            if( (srbDiskInfo.SRB_Status == SS_COMP) &&
                (srbDiskInfo.SRB_Int13HDriveInfo == c_drive) )
            {
                /* Make sure this is a cdrom device */
                struct SRB_GDEVBlock srbGDEVBlock;

                memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
                srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
                srbGDEVBlock.SRB_HaId   = i;
                srbGDEVBlock.SRB_Target = j;

                lpSendCommand( (void*) &srbGDEVBlock );

                if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
                    ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) )
                {
                    fd->i_sid = MAKEWORD( i, j );
                    dvdcss->i_fd = (int) fd;
                    dvdcss->i_pos = 0;
                    return 0;
                }
                else
                {
                    free( (void*) fd );
                    FreeLibrary( hASPI );
                    print_error( dvdcss,"this is not a cdrom drive" );
                    return -1;
                }
            }
        }
    }

    free( (void*) fd );
    FreeLibrary( hASPI );
    print_error( dvdcss, "unable to get haid and target (aspi)" );
    return -1;
}
#endif

#ifdef SYS_OS2
static int os2_open ( dvdcss_t dvdcss, char const *psz_device )
{
    char  psz_dvd[] = "X:";
    HFILE hfile;
    ULONG ulAction;
    ULONG rc;

    psz_dvd[0] = psz_device[0];

    rc = DosOpenL( ( PSZ )psz_dvd, &hfile, &ulAction, 0, FILE_NORMAL,
                   OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                   OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE | OPEN_FLAGS_DASD,
                   NULL );

    if( rc )
    {
        print_error( dvdcss, "failed to open device" );
        return -1;
    }

    setmode( hfile, O_BINARY );

    dvdcss->i_fd = dvdcss->i_read_fd = hfile;

    dvdcss->i_pos = 0;

    return 0;
}
#endif

/*****************************************************************************
 * Seek commands.
 *****************************************************************************/
static int libc_seek( dvdcss_t dvdcss, int i_blocks )
{
    off_t   i_seek;

    if( dvdcss->i_pos == i_blocks )
    {
        /* We are already in position */
        return i_blocks;
    }

    i_seek = (off_t)i_blocks * (off_t)DVDCSS_BLOCK_SIZE;
    i_seek = lseek( dvdcss->i_read_fd, i_seek, SEEK_SET );

    if( i_seek < 0 )
    {
        print_error( dvdcss, "seek error" );
        dvdcss->i_pos = -1;
        return i_seek;
    }

    dvdcss->i_pos = i_seek / DVDCSS_BLOCK_SIZE;

    return dvdcss->i_pos;
}

#if defined( WIN32 )
static int win2k_seek( dvdcss_t dvdcss, int i_blocks )
{
    LARGE_INTEGER li_seek;

#ifndef INVALID_SET_FILE_POINTER
#   define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

    if( dvdcss->i_pos == i_blocks )
    {
        /* We are already in position */
        return i_blocks;
    }

    li_seek.QuadPart = (LONGLONG)i_blocks * DVDCSS_BLOCK_SIZE;

    li_seek.LowPart = SetFilePointer( (HANDLE) dvdcss->i_fd,
                                      li_seek.LowPart,
                                      &li_seek.HighPart, FILE_BEGIN );
    if( (li_seek.LowPart == INVALID_SET_FILE_POINTER)
        && GetLastError() != NO_ERROR)
    {
        dvdcss->i_pos = -1;
        return -1;
    }

    dvdcss->i_pos = li_seek.QuadPart / DVDCSS_BLOCK_SIZE;

    return dvdcss->i_pos;
}

static int aspi_seek( dvdcss_t dvdcss, int i_blocks )
{
    int i_old_blocks;
    char sz_buf[ DVDCSS_BLOCK_SIZE ];
    struct w32_aspidev *fd = (struct w32_aspidev *) dvdcss->i_fd;

    if( dvdcss->i_pos == i_blocks )
    {
        /* We are already in position */
        return i_blocks;
    }

    i_old_blocks = fd->i_blocks;
    fd->i_blocks = i_blocks;

    if( aspi_read_internal( dvdcss->i_fd, sz_buf, 1 ) == -1 )
    {
        fd->i_blocks = i_old_blocks;
        dvdcss->i_pos = -1;
        return -1;
    }

    (fd->i_blocks)--;

    dvdcss->i_pos = fd->i_blocks;

    return dvdcss->i_pos;
}
#endif

/*****************************************************************************
 * Read commands.
 *****************************************************************************/
static int libc_read ( dvdcss_t dvdcss, void *p_buffer, int i_blocks )
{
    off_t i_size, i_ret;

    i_size = (off_t)i_blocks * (off_t)DVDCSS_BLOCK_SIZE;
    i_ret = read( dvdcss->i_read_fd, p_buffer, i_size );

    if( i_ret < 0 )
    {
        print_error( dvdcss, "read error" );
        dvdcss->i_pos = -1;
        return i_ret;
    }

    /* Handle partial reads */
    if( i_ret != i_size )
    {
        int i_seek;

        dvdcss->i_pos = -1;
        i_seek = libc_seek( dvdcss, i_ret / DVDCSS_BLOCK_SIZE );
        if( i_seek < 0 )
        {
            return i_seek;
        }

        /* We have to return now so that i_pos isn't clobbered */
        return i_ret / DVDCSS_BLOCK_SIZE;
    }

    dvdcss->i_pos += i_ret / DVDCSS_BLOCK_SIZE;
    return i_ret / DVDCSS_BLOCK_SIZE;
}

#if defined( WIN32 )
static int win2k_read ( dvdcss_t dvdcss, void *p_buffer, int i_blocks )
{
    int i_bytes;

    if( !ReadFile( (HANDLE) dvdcss->i_fd, p_buffer,
              i_blocks * DVDCSS_BLOCK_SIZE,
              (LPDWORD)&i_bytes, NULL ) )
    {
        dvdcss->i_pos = -1;
        return -1;
    }

    dvdcss->i_pos += i_bytes / DVDCSS_BLOCK_SIZE;
    return i_bytes / DVDCSS_BLOCK_SIZE;
}

static int aspi_read ( dvdcss_t dvdcss, void *p_buffer, int i_blocks )
{
    int i_read = aspi_read_internal( dvdcss->i_fd, p_buffer, i_blocks );

    if( i_read < 0 )
    {
        dvdcss->i_pos = -1;
        return i_read;
    }

    dvdcss->i_pos += i_read;
    return i_read;
}
#endif

/*****************************************************************************
 * Readv commands.
 *****************************************************************************/
static int libc_readv ( dvdcss_t dvdcss, struct iovec *p_iovec, int i_blocks )
{
#if defined( WIN32 ) || defined( GEKKO )
    int i_index, i_len, i_total = 0;
    unsigned char *p_base;
    int i_bytes;

    for( i_index = i_blocks;
         i_index;
         i_index--, p_iovec++ )
    {
        i_len  = p_iovec->iov_len;
        p_base = p_iovec->iov_base;

        if( i_len <= 0 )
        {
            continue;
        }

        i_bytes = read( dvdcss->i_fd, p_base, i_len );

        if( i_bytes < 0 )
        {
            /* One of the reads failed, too bad.
             * We won't even bother returning the reads that went ok,
             * and as in the posix spec the file postition is left
             * unspecified after a failure */
            dvdcss->i_pos = -1;
            return -1;
        }

        i_total += i_bytes;

        if( i_bytes != i_len )
        {
            /* We reached the end of the file or a signal interrupted
             * the read. Return a partial read. */
            int i_seek;

            dvdcss->i_pos = -1;
            i_seek = libc_seek( dvdcss, i_total / DVDCSS_BLOCK_SIZE );
            if( i_seek < 0 )
            {
                return i_seek;
            }

            /* We have to return now so that i_pos isn't clobbered */
            return i_total / DVDCSS_BLOCK_SIZE;
        }
    }

    dvdcss->i_pos += i_total / DVDCSS_BLOCK_SIZE;
    return i_total / DVDCSS_BLOCK_SIZE;
#else
    int i_read = readv( dvdcss->i_read_fd, p_iovec, i_blocks );

    if( i_read < 0 )
    {
        dvdcss->i_pos = -1;
        return i_read;
    }

    dvdcss->i_pos += i_read / DVDCSS_BLOCK_SIZE;
    return i_read / DVDCSS_BLOCK_SIZE;
#endif
}

#if defined( WIN32 )
/*****************************************************************************
 * win_readv: vectored read using ReadFile for Win2K and ASPI for win9x
 *****************************************************************************/
static int win_readv ( dvdcss_t dvdcss, struct iovec *p_iovec, int i_blocks )
{
    int i_index;
    int i_blocks_read, i_blocks_total = 0;

    /* Check the size of the readv temp buffer, just in case we need to
     * realloc something bigger */
    if( dvdcss->i_readv_buf_size < i_blocks * DVDCSS_BLOCK_SIZE )
    {
        dvdcss->i_readv_buf_size = i_blocks * DVDCSS_BLOCK_SIZE;

        if( dvdcss->p_readv_buffer ) free( dvdcss->p_readv_buffer );

        /* Allocate a buffer which will be used as a temporary storage
         * for readv */
        dvdcss->p_readv_buffer = malloc( dvdcss->i_readv_buf_size );
        if( !dvdcss->p_readv_buffer )
        {
            print_error( dvdcss, " failed (readv)" );
            dvdcss->i_pos = -1;
            return -1;
        }
    }

    for( i_index = i_blocks; i_index; i_index-- )
    {
        i_blocks_total += p_iovec[i_index-1].iov_len;
    }

    if( i_blocks_total <= 0 ) return 0;

    i_blocks_total /= DVDCSS_BLOCK_SIZE;

    if( WIN2K )
    {
        unsigned long int i_bytes;
        if( !ReadFile( (HANDLE)dvdcss->i_fd, dvdcss->p_readv_buffer,
                       i_blocks_total * DVDCSS_BLOCK_SIZE, &i_bytes, NULL ) )
        {
            /* The read failed... too bad.
             * As in the posix spec the file postition is left
             * unspecified after a failure */
            dvdcss->i_pos = -1;
            return -1;
        }
        i_blocks_read = i_bytes / DVDCSS_BLOCK_SIZE;
    }
    else /* Win9x */
    {
        i_blocks_read = aspi_read_internal( dvdcss->i_fd,
                                            dvdcss->p_readv_buffer,
                                            i_blocks_total );
        if( i_blocks_read < 0 )
        {
            /* See above */
            dvdcss->i_pos = -1;
            return -1;
        }
    }

    /* We just have to copy the content of the temp buffer into the iovecs */
    for( i_index = 0, i_blocks_total = i_blocks_read;
         i_blocks_total > 0;
         i_index++ )
    {
        memcpy( p_iovec[i_index].iov_base,
                dvdcss->p_readv_buffer + (i_blocks_read - i_blocks_total)
                                           * DVDCSS_BLOCK_SIZE,
                p_iovec[i_index].iov_len );
        /* if we read less blocks than asked, we'll just end up copying
         * garbage, this isn't an issue as we return the number of
         * blocks actually read */
        i_blocks_total -= ( p_iovec[i_index].iov_len / DVDCSS_BLOCK_SIZE );
    }

    dvdcss->i_pos += i_blocks_read;
    return i_blocks_read;
}

static int aspi_read_internal( int i_fd, void *p_data, int i_blocks )
{
    HANDLE hEvent;
    struct SRB_ExecSCSICmd ssc;
    struct w32_aspidev *fd = (struct w32_aspidev *) i_fd;

    /* Create the transfer completion event */
    hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( hEvent == NULL )
    {
        return -1;
    }

    memset( &ssc, 0, sizeof( ssc ) );

    ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;
    ssc.SRB_Flags       = SRB_DIR_IN | SRB_EVENT_NOTIFY;
    ssc.SRB_HaId        = LOBYTE( fd->i_sid );
    ssc.SRB_Target      = HIBYTE( fd->i_sid );
    ssc.SRB_SenseLen    = SENSE_LEN;

    ssc.SRB_PostProc = (LPVOID) hEvent;
    ssc.SRB_BufPointer  = p_data;
    ssc.SRB_CDBLen      = 12;

    ssc.CDBByte[0]      = 0xA8; /* RAW */
    ssc.CDBByte[2]      = (UCHAR) (fd->i_blocks >> 24);
    ssc.CDBByte[3]      = (UCHAR) (fd->i_blocks >> 16) & 0xff;
    ssc.CDBByte[4]      = (UCHAR) (fd->i_blocks >> 8) & 0xff;
    ssc.CDBByte[5]      = (UCHAR) (fd->i_blocks) & 0xff;

    /* We have to break down the reads into 64kb pieces (ASPI restriction) */
    if( i_blocks > 32 )
    {
        ssc.SRB_BufLen = 32 * DVDCSS_BLOCK_SIZE;
        ssc.CDBByte[9] = 32;
        fd->i_blocks  += 32;

        /* Initiate transfer */
        ResetEvent( hEvent );
        fd->lpSendCommand( (void*) &ssc );

        /* transfer the next 64kb (aspi_read_internal is called recursively)
         * We need to check the status of the read on return */
        if( aspi_read_internal( i_fd,
                                (uint8_t*) p_data + 32 * DVDCSS_BLOCK_SIZE,
                                i_blocks - 32) < 0 )
        {
            return -1;
        }
    }
    else
    {
        /* This is the last transfer */
        ssc.SRB_BufLen   = i_blocks * DVDCSS_BLOCK_SIZE;
        ssc.CDBByte[9]   = (UCHAR) i_blocks;
        fd->i_blocks += i_blocks;

        /* Initiate transfer */
        ResetEvent( hEvent );
        fd->lpSendCommand( (void*) &ssc );

    }

    /* If the command has still not been processed, wait until it's finished */
    if( ssc.SRB_Status == SS_PENDING )
    {
        WaitForSingleObject( hEvent, INFINITE );
    }
    CloseHandle( hEvent );

    /* check that the transfer went as planned */
    if( ssc.SRB_Status != SS_COMP )
    {
      return -1;
    }

    return i_blocks;
}
#endif

#ifdef GEKKO
static int di_open(dvdcss_t dvdcss, char const * psz_device) {

	if (strcmp(psz_device, "/dev/di")) {
		print_debug(dvdcss, "DI: unknown device '%s'", psz_device);
		return -1;
	}

	if(!StartDVDMotor())
		return -1;

	dvdcss->i_pos = 0;
	return 0;
}

static int di_seek(dvdcss_t dvdcss, int i_blocks) {
	if (dvdcss->i_pos == i_blocks )
		return i_blocks;

	//TODO check this against the disc's max lba
	dvdcss->i_pos = i_blocks;
	return dvdcss->i_pos;
}

static int di_read(dvdcss_t dvdcss, void *buffer, int blocks) {
	void *bfr;
	int ret;

	bfr = memalign(32, blocks * DVDCSS_BLOCK_SIZE);
	if (bfr == NULL) {
		print_debug(dvdcss, "DI: can't allocate %d bytes",
					blocks * DVDCSS_BLOCK_SIZE);
		return -1;
	}

	ret = DI_ReadDVD(bfr, blocks, dvdcss->i_pos);

	if (ret < 0) {
		print_debug(dvdcss, "DI: read failed with %d", ret);
		free(bfr);
		return -1;
	}

	dvdcss->i_pos += blocks;
	memcpy(buffer, bfr, blocks * DVDCSS_BLOCK_SIZE);
	free(bfr);

	return blocks;
}

static int di_readv(dvdcss_t dvdcss, struct iovec *iov, int iovcnt) {
	int len;
	int i;
	int ret;
	void *bfr, *ptr;

	len = 0;
	for (i = 0; i < iovcnt; ++i)
		len += iov[i].iov_len;

	bfr = memalign(32, len);
	if (bfr == NULL) {
		print_debug(dvdcss, "DI: can't allocate %d bytes",
					len * DVDCSS_BLOCK_SIZE);
		return -1;
	}

	ret = DI_ReadDVD(bfr, len / DVDCSS_BLOCK_SIZE, dvdcss->i_pos);
	if (ret < 0) {
		print_debug( dvdcss, "DI: readv failed with %d", ret);
		free(bfr);
		return -1;
	}

	ptr = bfr;
	for (i = 0; i < iovcnt; ++i) {
		memcpy(iov[i].iov_base, ptr, iov[i].iov_len);
		ptr += iov[i].iov_len;
	}

	dvdcss->i_pos += len / DVDCSS_BLOCK_SIZE;
	free(bfr);

	return len;
}

#ifdef GEKKO
#define BUFFER_SECTOR_SIZE 16
static char *read_buffer = NULL;
static int cache_start;
static int cache_sectors;
#endif

static int usb_open(dvdcss_t dvdcss, char const * psz_device)
{
	if (strcmp(psz_device, "/dev/usb")) {
		print_debug(dvdcss, "USB: unknown device '%s'", psz_device);
		return -1;
	}

	if (usb->isInserted() == 0 || USBStorage_IsDVD() == 0)
		return -1;

	if(!read_buffer)
		read_buffer = mem2_memalign(32, BUFFER_SECTOR_SIZE*DVDCSS_BLOCK_SIZE, MEM2_OTHER);

	dvdcss->i_pos = 0;	
	cache_sectors = 0;
	return 0;
}

static int usb_seek(dvdcss_t dvdcss, int i_blocks)
{
	dvdcss->i_pos = i_blocks;
	return dvdcss->i_pos;
}

static int __read(dvdcss_t dvdcss, void *ptr, int start, int sectors)
{
	sectors = (BUFFER_SECTOR_SIZE < sectors) ? BUFFER_SECTOR_SIZE : sectors;
	if (cache_sectors && start >= cache_start && (start + sectors) <= (cache_start + cache_sectors))
	{
		memcpy(ptr, read_buffer+(start-cache_start)*DVDCSS_BLOCK_SIZE, sectors*DVDCSS_BLOCK_SIZE);
		return sectors;
	}

	if (!usb->readSectors(start, BUFFER_SECTOR_SIZE, read_buffer))
	{
		cache_sectors = 0;
		return -1;
	}

	cache_start = start;
	cache_sectors = BUFFER_SECTOR_SIZE;
	memcpy(ptr, read_buffer, sectors*DVDCSS_BLOCK_SIZE);

	return sectors;
}

static int _read(dvdcss_t dvdcss, void *ptr, int start, int sectors)
{
	int ret, done = 0;
	char *cptr = ptr;
	while (done < sectors)
	{
		ret = __read(dvdcss, cptr+done*DVDCSS_BLOCK_SIZE, start+done, sectors-done);
		if (ret > 0) {
			done += ret;
		}
		else if (ret == 0)
			break;
		else
			return -1;
	}
	return done;
}

static int usb_read(dvdcss_t dvdcss, void *buffer, int blocks)
{
	int ret;

    ret = _read(dvdcss, buffer, dvdcss->i_pos, blocks);
	if (ret < 0) {
		print_debug(dvdcss, "USB: read failed with %d", ret);
		return -1;
	}

	dvdcss->i_pos += ret;
	return ret;
}

static int usb_readv(dvdcss_t dvdcss, struct iovec *iov, int iovcnt)
{
	int len;
	int i;
	void *bfr, *ptr;
	int ret;

	len = 0;
	for (i = 0; i < iovcnt; ++i)
		len += iov[i].iov_len;

	bfr = memalign(32, len);
	if (bfr == NULL) {
		print_debug(dvdcss, "USB: can't allocate %d bytes", len);
		return -1;
	}

    ret = _read(dvdcss, bfr, dvdcss->i_pos, len/DVDCSS_BLOCK_SIZE);
	if (ret < 0) {
		print_debug(dvdcss, "USB: read failed with %d", ret);
		free(bfr);
		return -1;
	}

	ptr = bfr;
	for (i = 0; i < iovcnt; ++i) {
		memcpy(iov[i].iov_base, ptr, iov[i].iov_len);
		ptr += iov[i].iov_len;
	}

	dvdcss->i_pos += len/DVDCSS_BLOCK_SIZE;
	free(bfr);

	return len;
}
#endif

