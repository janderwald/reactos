/*
* PROJECT:     ReactOS Universal Video Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbvideo/uvc.c
* PURPOSE:     USB Video device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/
#define NDEBUG
#include "usbvideo.h"

VOID
NTAPI
USBVideoDeliverFrame(
    PUSB_VIDEO_DEVICE_EXTENSION DeviceExtension,
    PUCHAR FrameBuffer,
    ULONG FrameSize,
    UCHAR CompleteFrame)
{
    PKSSTREAM_POINTER StreamPointer;
    PKSPIN Pin;
    KIRQL OldLevel;

    if (!DeviceExtension || !DeviceExtension->Pin)
    {
        DPRINT("USBVideoDeliverFrame: Invalid device extension or pin\n");
        return;
    }

    Pin = DeviceExtension->Pin;

    /* Get leading edge stream pointer - this is the user's buffer from IOCTL_KS_READ_STREAM */
    StreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
    if (!StreamPointer)
    {
        DPRINT("USBVideoDeliverFrame: No stream pointer available\n");
        return;
    }

    if (!StreamPointer->StreamHeader || !StreamPointer->StreamHeader->Data)
    {
        DPRINT("USBVideoDeliverFrame: No stream header or data pointer\n");
        KsStreamPointerUnlock(StreamPointer, FALSE);
        return;
    }

    /* Copy frame data to user buffer */
    ULONG BytesToCopy = min(StreamPointer->Offset->Remaining, FrameSize);
    if (BytesToCopy > 0)
    {
        //ASSERT(BytesToCopy == FrameSize);
        //ASSERT(FrameSize < StreamPointer->Offset->Remaining);
        ULONG Offset = StreamPointer->Offset->Count - StreamPointer->Offset->Remaining;
        DPRINT("USBVideoDeliverFrame: Copying %u bytes at offset %u user buffer (frame size %u)\n",
            BytesToCopy,
            Offset,
            FrameSize);

        KeAcquireSpinLock(&DeviceExtension->StreamingLock, &OldLevel);
        RtlCopyMemory(&((PUCHAR)StreamPointer->StreamHeader->Data)[Offset], FrameBuffer, BytesToCopy);
        KeReleaseSpinLock(&DeviceExtension->StreamingLock, OldLevel);

        /* Advance the stream pointer to deliver the buffer */
        KsStreamPointerAdvanceOffsetsAndUnlock(StreamPointer, 0, BytesToCopy, CompleteFrame);

    }
    else
    {
        DPRINT("USBVideoDeliverFrame: BytesToCopy is 0\n");
        KsStreamPointerUnlock(StreamPointer, FALSE);
    }
}
