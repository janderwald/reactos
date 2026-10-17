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

VOID
UsbVideoProcessStatusPacket(
    PUCHAR pData,
    ULONG  length)
{
    if (length < 2) return;

    UCHAR bStatusType = pData[0];
    UCHAR bOriginator = pData[1];
    UCHAR bEvent      = length > 2 ? pData[2] : 0;
    UCHAR bSelector   = length > 3 ? pData[3] : 0;
    UCHAR bAttribute  = length > 4 ? pData[4] : 0;

    UCHAR originType  = bStatusType & 0x0F;
    DPRINT("bStatusType %x bOriginator %x bEvent %x bSelector %x bAttribute %x\n",
        bStatusType, bOriginator, bEvent, bSelector, bAttribute);

    switch (originType) {
        case 0x01:  // VideoControl Interface
            DPRINT("UVC VC Status: Unit=%d Event=0x%02X "
                     "Selector=0x%02X Attr=0x%02X\n",
                     bOriginator, bEvent, bSelector, bAttribute);

            // Control Change Event
            if (bEvent == 0x00) {
                switch (bSelector) {
                    case 0x01:  // VS_PROBE_CONTROL
                        DPRINT1("  Probe Control geaendert\n");
                        ASSERT(FALSE);
                        break;
                    case 0x02:  // VS_COMMIT_CONTROL
                        DPRINT1("  Commit Control geaendert\n");
                        ASSERT(FALSE);
                        break;
                }
            }
            break;

        case 0x02:  // VideoStreaming Interface
            DPRINT("UVC VS Status: Terminal=%d Event=0x%02X\n",
                     bOriginator, bEvent);
            break;

        default:
            DPRINT("UVC Status unbekannt: Type=0x%02X\n",
                     bStatusType);
            break;
    }
}


NTSTATUS
USBVideoReadErrorControl(
    IN PUSB_VIDEO_DEVICE_EXTENSION DeviceExtension
)
{
    NTSTATUS Status;
    UCHAR ErrorResponse[16] = {0};
    UCHAR GetCurFormat = 0x81;
    USHORT CurFormatValue = 0x600;
    UCHAR InterfaceNumber;
    PURB Urb;

    Status = USBVideoGetInterfaceDescriptorForStreaming(DeviceExtension, &InterfaceNumber);
    if (!NT_SUCCESS(Status))
    {
        // failed to retrieve interface number
        DPRINT1("USBVideoGetInterfaceDescriptorForStreaming failed\n");
        return Status;
    }
    Urb = AllocFunction(sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (!Urb)
    {
        DPRINT1("USBVIDEO Failed to allocate\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ErrorResponse, sizeof(ErrorResponse));
    UsbBuildInterruptOrBulkTransferRequest(
        Urb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
        DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle,
        ErrorResponse,
        NULL,
        sizeof(ErrorResponse),
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
        NULL
    );
    Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);
    if (NT_SUCCESS(Status))
    {
        UsbVideoProcessStatusPacket(ErrorResponse, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
    }
    FreeFunction(Urb);
    return Status;
}


NTSTATUS
NTAPI
USBVideoIsoReadFrameComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    PUSB_VIDEO_DEVICE_EXTENSION DeviceExtension;
    PURB Urb;
    PKSPIN Pin = (PKSPIN)Context;

    /* get device extension */
    DeviceExtension = Pin->Context;

    /* get urb */
    Urb = Irp->Tail.Overlay.DriverContext[0];

    /* store frame number */
    DeviceExtension->UsbFrameNumber = Urb->UrbGetCurrentFrameNumber.FrameNumber;

    /* set event */
    KeSetEvent(&DeviceExtension->UsbFrameNumberUpdated, 0, FALSE);

    /* free urb */
    FreeFunction(Urb);

    /* free irp */
    FreeFunction(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBVideoGetCurrentFrameNumber(
    IN PKSPIN Pin)
{
    PUSB_VIDEO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PIRP Irp;
    PURB pIsoFrameNumber;

    /* get device extension */
    DeviceExtension = Pin->Context;

    Irp = AllocFunction(IoSizeOfIrp(DeviceExtension->LowerDevice->StackSize));
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize irp */
    IoInitializeIrp(Irp, IoSizeOfIrp(DeviceExtension->LowerDevice->StackSize), DeviceExtension->LowerDevice->StackSize);

    /* set irp members */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;

    /* allocate urb */
    pIsoFrameNumber = AllocFunction(sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER));
    if (!pIsoFrameNumber)
    {
        FreeFunction(Irp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pIsoFrameNumber->UrbHeader.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;
    pIsoFrameNumber->UrbHeader.Length = sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER);

    /* setup next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.Others.Argument1 = pIsoFrameNumber;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Irp->Tail.Overlay.DriverContext[0] = pIsoFrameNumber;

    IoSetCompletionRoutine(Irp, USBVideoIsoReadFrameComplete, (PVOID)Pin, TRUE, TRUE, TRUE);
    return IoCallDriver(DeviceExtension->LowerDevice, Irp);
}


NTSTATUS
USBVideoQueueIsoRead(
    IN PKSPIN Pin,
    IN USBD_PIPE_HANDLE hIsoPipe,
    IN PUCHAR TransferBuffer,
    IN ULONG TransferLength,
    IN PIRP Irp,
    IN PURB Urb,
    IN ULONG FrameNumber,
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
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN  | USBD_START_ISO_TRANSFER_ASAP;
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
    KIRQL OldLevel;

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
                              0, // FIXME
                              Frame);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    KeAcquireSpinLock(&DeviceExtension->StreamingLock, &OldLevel);

    Data         = (PUCHAR)Urb->UrbIsochronousTransfer.TransferBuffer;
    for(Index = 0; Index < Urb->UrbIsochronousTransfer.NumberOfPackets; Index++)
    {
        if (Urb->UrbIsochronousTransfer.IsoPacket[Index].Status != USBD_STATUS_SUCCESS)
        {
            DPRINT("Status %x failed for packet %u\n", Urb->UrbIsochronousTransfer.IsoPacket[Index].Status, Index);
            continue;
        }
        Offset = Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset;
        BytesReceived = Urb->UrbIsochronousTransfer.IsoPacket[Index].Length;
        Hdr = (PUVC_PAYLOAD_HEADER)(Data + Offset);
        HeaderLen = Hdr->bHeaderLength;

        /* validate header length */
        if (HeaderLen < 2 || HeaderLen > 12 || HeaderLen >= BytesReceived)
        {
            DPRINT("Invalid packet atIso Index %u Offset %u HeaderLen %u BytesReceived %u\n", Index,
                    Offset, HeaderLen, BytesReceived);
            continue;
        }

        /* discard frame if err bit is set */
        if (DeviceExtension->IsMjpegFormat)
        {
            if (Hdr->ERR) {
                DPRINT1("ERR bit set at IsoPacket Index %u BytesReceived %u\n", Index, BytesReceived);//, Value[0], Status);
                //KsQueueWorkItem(DeviceExtension->ErrorWorker, &DeviceExtension->ErrorWorkItem);
                Frame->FrameSize = 0;
                Frame->FrameStarted = FALSE;
                break;
            }
        }
        if (!DeviceExtension->IsMjpegFormat)
        {
            /* check FID */
#if 1
            if (Frame->FrameStarted && Frame->FrameSize > 0 && (Hdr->FID != Frame->LastFid))
            {
                USBVideoDeliverFrame(DeviceExtension,
                                Frame->FrameBuffer,
                                Frame->FrameSize,
                                TRUE);
                Frame->FrameSize    = 0;
                Frame->FrameStarted = FALSE;
            }
#endif
        }
        Frame->LastFid = Hdr->FID;
        Frame->dwPresentationTimestamp = Hdr->dwPresentationTime;

        /* copy payload */
        ULONG PayloadDataOffset = Offset + HeaderLen;
        ULONG Length = Urb->UrbIsochronousTransfer.IsoPacket[Index].Length;
        ASSERT(Length >= HeaderLen);
        ULONG PayloadDataLen    = Length - HeaderLen;

        if (DeviceExtension->IsMjpegFormat)
        {
            if (Data[PayloadDataOffset] == 0xFF &&
                Data[PayloadDataOffset+1] == 0xD8)
            {
                if (Frame->FrameStarted && Frame->FrameSize > 0)
                {
                    DPRINT1("SOF found at Offset %u PacketIndex %u\n", PayloadDataOffset, Index);
                    USBVideoDeliverFrame(DeviceExtension,
                                     Frame->FrameBuffer,
                                     Frame->FrameSize,
                                     TRUE);
                }
                Frame->FrameSize    = 0;
                Frame->FrameStarted = FALSE;
            }
            if (PayloadDataLen > 0 && Frame->FrameSize + PayloadDataLen <= Frame->MaxFrameSize)
            {
                DPRINT("Copying Bytes %u\n", PayloadDataLen);
                RtlCopyMemory(
                    Frame->FrameBuffer + Frame->FrameSize,
                    Data + PayloadDataOffset,
                    PayloadDataLen);

                /* update buffer */
                Frame->FrameSize   += PayloadDataLen;
                Frame->FrameStarted = TRUE;
            }
        }
        else
        {
            if (PayloadDataLen > 0 && Frame->FrameSize + PayloadDataLen <= Frame->MaxFrameSize)
            {
                DPRINT("Copying Bytes %u\n", PayloadDataLen);
                RtlCopyMemory(
                    Frame->FrameBuffer + Frame->FrameSize,
                    Data + PayloadDataOffset,
                    PayloadDataLen);

                /* update buffer */
                Frame->FrameSize   += PayloadDataLen;
                Frame->FrameStarted = TRUE;
            }
        }

        if (DeviceExtension->IsMjpegFormat)
        {
            if (Frame->FrameSize > 2 && Frame->FrameBuffer[Frame->FrameSize-2] == 0xFF &&
                Frame->FrameBuffer[Frame->FrameSize-1] == 0xD9)
            {
                DPRINT1("EOF found at Offset %u PacketIndex %u EOF %x\n", PayloadDataOffset, Index, Hdr->EOF);
                USBVideoDeliverFrame(DeviceExtension,
                                    Frame->FrameBuffer,
                                    Frame->FrameSize,
                                    TRUE);
                Frame->FrameSize    = 0;
                Frame->FrameStarted = FALSE;
                break;
            }
        }
#if 0
        /* check FID */
        if (Frame->FrameStarted && Frame->FrameSize > 2 && Hdr->EOF)
        {
            USBVideoDeliverFrame(DeviceExtension,
                                Frame->FrameBuffer,
                                Frame->FrameSize,
                                TRUE);
            Frame->FrameSize    = 0;
            Frame->FrameStarted = FALSE;
        }
#endif
        if (Frame->FrameSize == Frame->MaxFrameSize || Hdr->EOF)
        {
            if (DeviceExtension->IsMjpegFormat == FALSE)
            {
                 USBVideoDeliverFrame(DeviceExtension,
                                 Frame->FrameBuffer,
                                  Frame->FrameSize,
                                  TRUE);

                Frame->FrameSize    = 0;
                Frame->FrameStarted = FALSE;
                break;
            }
        }
    }
    KeReleaseSpinLock(&DeviceExtension->StreamingLock, OldLevel);

    //RtlZeroMemory(Urb->UrbIsochronousTransfer.TransferBuffer, DeviceExtension->IsoTransferSize);
    //USBVideoGetCurrentFrameNumber(Pin);
    //DPRINT1("[USBVIDEO] UsbFrameNumber %u\n", DeviceExtension->UsbFrameNumber);
    /* requeue irp */
    DPRINT("[USBVIDEO] Rescheduling...\n");
    USBVideoQueueIsoRead(Pin,
                         DeviceExtension->hPipe,
                         (PUCHAR)Urb->UrbIsochronousTransfer.TransferBuffer,
                         DeviceExtension->IsoTransferSize,
                         Irp,
                         Urb,
                         DeviceExtension->UsbFrameNumber,
                         Frame);




    // KeAcquireSpinLock(&DeviceExtension->IsoIrpCompletionLock, &OldLevel);
    // InsertTailList(&DeviceExtension->IsoIrpCompletionList, &Irp->Tail.Overlay.ListEntry);
    // DeviceExtension->IsoIrpCompletionCount++;
    // KeReleaseSpinLock(&DeviceExtension->IsoIrpCompletionLock, OldLevel);

    // if (DeviceExtension->IsoIrpCompletionCount == DeviceExtension->UrbPoolCount)
    // {
    //     USBVideoGetCurrentFrameNumber(Pin);
    //     //KeWaitForSingleObject(&DeviceExtension->UsbFrameNumberUpdated, Executive, KernelMode, FALSE, NULL);

    //     KeAcquireSpinLock(&DeviceExtension->IsoIrpCompletionLock, &OldLevel);
    //     while(!IsListEmpty(&DeviceExtension->IsoIrpCompletionList))
    //     {
    //         PLIST_ENTRY Entry = RemoveHeadList(&DeviceExtension->IsoIrpCompletionList);
    //         PIRP IrpEntry = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
    //         PURB pUrb = IrpEntry->Tail.Overlay.DriverContext[0];
    //         PFRAME_CONTEXT pFrame = IrpEntry->Tail.Overlay.DriverContext[1];

    //         KeReleaseSpinLock(&DeviceExtension->IsoIrpCompletionLock, OldLevel);
    //         DeviceExtension->IsoIrpCompletionCount--;
    //         /* requeue irp */
    //         USBVideoQueueIsoRead(Pin,
    //                              DeviceExtension->hPipe,
    //                             (PUCHAR)pUrb->UrbIsochronousTransfer.TransferBuffer,
    //                              DeviceExtension->IsoTransferSize,
    //                             IrpEntry,
    //                             pUrb,
    //                             DeviceExtension->UsbFrameNumber,
    //                             pFrame);

    //         KeAcquireSpinLock(&DeviceExtension->IsoIrpCompletionLock, &OldLevel);
    //     }
    //     KeReleaseSpinLock(&DeviceExtension->IsoIrpCompletionLock, OldLevel);
    // }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

