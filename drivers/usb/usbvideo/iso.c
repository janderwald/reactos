/*
* PROJECT:     ReactOS Universal Video Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbvideo/iso.c
* PURPOSE:     USB Video device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/
#define NDEBUG
#include "usbvideo.h"

NTSTATUS
USBVideoQueueIsoRead(
    IN PKSPIN Pin,
    IN USBD_PIPE_HANDLE hIsoPipe,
    IN PUCHAR TransferBuffer,
    IN ULONG TransferLength,
    IN PIRP Irp,
    IN PURB Urb,
    IN PFRAME_CONTEXT FrameCtx)
{
    PUSB_VIDEO_DEVICE_EXTENSION DeviceExtension;
    ULONG Index;
    PIO_STACK_LOCATION IoStack;

    /* get device extension */
    DeviceExtension = Pin->Context;

    if (DeviceExtension->StopStreaming)
    {
        InterlockedIncrement(&DeviceExtension->StoppedStreamingIrps);

        if (InterlockedCompareExchange(&DeviceExtension->StoppedStreamingIrps, 0, DeviceExtension->UrbPoolCount))
        {
            KeSetEvent(&DeviceExtension->StoppedStreamingEvent, 0, FALSE);
        }
        return STATUS_SUCCESS;
    }

    /* initialize irp */
    IoInitializeIrp(Irp, IoSizeOfIrp(DeviceExtension->LowerDevice->StackSize), DeviceExtension->LowerDevice->StackSize);

    /* set irp members */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;

    RtlZeroMemory(Urb, GET_ISO_URB_SIZE(DeviceExtension->IsoPacketCount));

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = GET_ISO_URB_SIZE(DeviceExtension->IsoPacketCount);
    Urb->UrbIsochronousTransfer.PipeHandle = DeviceExtension->hPipe;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = (DeviceExtension->dwMaxPayloadTransferSize) * DeviceExtension->IsoPacketCount;
    Urb->UrbIsochronousTransfer.TransferBuffer = TransferBuffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = DeviceExtension->IsoPacketCount;
    Urb->UrbIsochronousTransfer.StartFrame = 0;

    for (Index = 0; Index < DeviceExtension->IsoPacketCount; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * (DeviceExtension->dwMaxPayloadTransferSize);
    }
    /* setup next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = Urb;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Irp->Tail.Overlay.DriverContext[0] = Urb;
    Irp->Tail.Overlay.DriverContext[1] = FrameCtx;

    IoSetCompletionRoutine(Irp, USBVideoIsoReadComplete, (PVOID)Pin, TRUE, TRUE, TRUE);
    return IoCallDriver(DeviceExtension->LowerDevice, Irp);
}

NTSTATUS
NTAPI
USBVideoIsoReadComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    ULONG  BytesReceived;
    PUCHAR Data;
    ULONG  Offset = 0;
    PUVC_PAYLOAD_HEADER Hdr;
    UCHAR HeaderLen;
    ULONG Index;
    PKSPIN Pin = (PKSPIN)Context;

    PUSB_VIDEO_DEVICE_EXTENSION  DeviceExtension = Pin->Context;
    PURB Urb = Irp->Tail.Overlay.DriverContext[0];
    PFRAME_CONTEXT Frame = Irp->Tail.Overlay.DriverContext[1];

    /* check for success */
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("Irp Failed with %x\n", Irp->IoStatus.Status);
        USBVideoQueueIsoRead(Pin,
                              DeviceExtension->hPipe,
                              (PUCHAR)Urb->UrbIsochronousTransfer.TransferBuffer,
                              DeviceExtension->IsoTransferSize,
                              Irp,
                              Urb,
                              Frame);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    BytesReceived = 0;
    UCHAR FirstFID = 0xFF, NewFid = 0xFF;
    ULONG PacketFIDChange = -1;
    Data         = (PUCHAR)Urb->UrbIsochronousTransfer.TransferBuffer;

    for(Index = 0; Index < Urb->UrbIsochronousTransfer.NumberOfPackets; Index++)
    {
        //DPRINT1("Index %u Length %u\n", Index, Urb->UrbIsochronousTransfer.IsoPacket[Index].Length);
        BytesReceived += Urb->UrbIsochronousTransfer.IsoPacket[Index].Length;
        if (Urb->UrbIsochronousTransfer.IsoPacket[Index].Status != USBD_STATUS_SUCCESS)
        {
            DPRINT("Status %x failed for packet %u\n", Urb->UrbIsochronousTransfer.IsoPacket[Index].Status, Index);
            continue;
        }
        Offset = Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset;
        Hdr = (PUVC_PAYLOAD_HEADER)(Data + Offset);
        HeaderLen = Hdr->bHeaderLength;
        /* validate header length */
        if (HeaderLen < 2 || HeaderLen > 12 || Offset + HeaderLen > BytesReceived)
        {
            DPRINT("Invalid packet atIso Index %u Offset %u HeaderLen %u BytesReceived %u\n", Index,
                    Offset, HeaderLen, BytesReceived);
            continue;
        }
        if (FirstFID == 0xFF)
        {
            /* valid first packet */
            FirstFID = Hdr->FID;
            continue;
        }
        if (FirstFID != Hdr->FID)
        {
            if (PacketFIDChange == (ULONG)-1)
            {
                PUCHAR DataBuffer = (PUCHAR)(Data + Offset + HeaderLen);
                if (DataBuffer[0] == 0xFF && DataBuffer[1] == 0xD8)
                {
                    /* fid changed at packet */
                    PacketFIDChange = Index;
                    NewFid = Hdr->FID;
                }
            }
        }
    }

    Offset        = 0;
    if (PacketFIDChange != (ULONG)-1)
    {
          Index = PacketFIDChange;
          Frame->LastFid = NewFid;
          Frame->FrameSize = 0;
          Frame->FrameStarted = TRUE;
          //DPRINT1("FIDChange at Packet Index %u\n", Index);
    }
    else
    {
        Index = Urb->UrbIsochronousTransfer.NumberOfPackets + 1; // throw away packet
    }
    for(; Index < Urb->UrbIsochronousTransfer.NumberOfPackets; Index++)
    {
        if (Urb->UrbIsochronousTransfer.IsoPacket[Index].Status != USBD_STATUS_SUCCESS)
        {
            DPRINT("Status %x failed for packet %u\n", Urb->UrbIsochronousTransfer.IsoPacket[Index].Status, Index);
            continue;
        }
        Offset = Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset;
        Hdr = (PUVC_PAYLOAD_HEADER)(Data + Offset);
        HeaderLen = Hdr->bHeaderLength;

        /* validate header length */
        if (HeaderLen < 2 || HeaderLen > 12 || Offset + HeaderLen > BytesReceived)
        {
            DPRINT("Invalid packet atIso Index %u Offset %u HeaderLen %u BytesReceived %u\n", Index,
                    Offset, HeaderLen, BytesReceived);
            continue;
        }

        /* discard packet if EOH bit is not set */
        if (Hdr->EOH == 0) {
            DPRINT1("EOH bit not set at IsoPacket Index %u\n", Index);//, Value[0], Status);
            continue;
        }

        /* discard packet if RES bit is set */
        if (Hdr->RES) {
            DPRINT1("RES bit set at IsoPacket Index %u\n", Index);//, Value[0], Status);
            continue;
        }

        /* discard frame if err bit is set */
        if (Hdr->ERR) {
            DPRINT1("ERR bit set at IsoPacket Index %u\n", Index);//, Value[0], Status);
            Frame->FrameStarted = FALSE;
            Frame->FrameSize    = 0;
            continue;
        }
        /* check FID */
        if (Frame->FrameStarted && Frame->FrameSize > 0 && (Hdr->FID != Frame->LastFid)) {
            USBVideoDeliverFrame(DeviceExtension,
                            Frame->FrameBuffer,
                            Frame->FrameSize);
            Frame->FrameSize    = 0;
            Frame->FrameStarted = FALSE;
        }
        Frame->LastFid = Hdr->FID;

        /* copy payload */
        ULONG PayloadDataOffset = Offset + HeaderLen;
        ULONG Length = Urb->UrbIsochronousTransfer.IsoPacket[Index].Length;
        ULONG PayloadDataLen    = Length - HeaderLen;
#if 0
        /* validate payload length*/
        if (PayloadDataOffset + PayloadDataLen > BytesReceived)
            PayloadDataLen = BytesReceived - PayloadDataOffset;
#endif
        if (PayloadDataLen > 0 && Frame->FrameSize + PayloadDataLen <= Frame->MaxFrameSize) {

            RtlCopyMemory(
                Frame->FrameBuffer + Frame->FrameSize,
                Data + PayloadDataOffset,
                PayloadDataLen);

            /* update buffer */
            Frame->FrameSize   += PayloadDataLen;
            Frame->FrameStarted = TRUE;
        }

        /* check FID */
        if (Frame->FrameStarted && Frame->FrameSize > 2 && Hdr->EOF) {
            if (Frame->FrameBuffer[Frame->FrameSize-2] == 0xFF &&
                Frame->FrameBuffer[Frame->FrameSize-1] == 0xD9)
            {
                USBVideoDeliverFrame(DeviceExtension,
                                Frame->FrameBuffer,
                                Frame->FrameSize);
                Frame->FrameSize    = 0;
                Frame->FrameStarted = FALSE;
            }
        }

        if (Frame->FrameSize == Frame->MaxFrameSize)
        {
            DPRINT1("Buffer full discarding\n");
            Frame->FrameSize    = 0;
            Frame->FrameStarted = FALSE;
        }
    }

    /* requeue irp */
    USBVideoQueueIsoRead(Pin,
                          DeviceExtension->hPipe,
                          (PUCHAR)Urb->UrbIsochronousTransfer.TransferBuffer,
                          DeviceExtension->IsoTransferSize,
                          Irp,
                          Urb,
                          Frame);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

