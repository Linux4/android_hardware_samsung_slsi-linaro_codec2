/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _EXYNOS_VIDEO_COMMON__API_H_
#define _EXYNOS_VIDEO_COMMON__API_H_

#include "ExynosVideo_OSAL.h"

int MFC_QueryCap(CodecOSALVideoContext *pCtx);
int MFC_Create_Epoll(CodecOSALVideoContext *pCtx, int padIndex);
int MFC_Pad_init(CodecOSALVideoContext *pCtx);
int MFC_SharedMem_init(CodecOSALVideoContext *pCtx);
int MFC_Init_Common(CodecOSALVideoContext *pCtx);

ExynosVideoErrorType MFC_Wait_Buffer_Common(void *pHandle, ExynosVideoPollType *avail, bool bIsEncode, PortType port);
ExynosVideoErrorType MFC_Wait_Buffer_Common_Epoll(void *pHandle, ExynosVideoPollType *avail, PortType port);
ExynosVideoErrorType MFC_Stop_Wait_Buffer(void *pHandle);
ExynosVideoErrorType MFC_Reset_Wait_Buffer(void *pHandle);

ExynosVideoErrorType MFC_Enable_Cacheable_Inbuf(void *pHandle);
ExynosVideoErrorType MFC_Enable_Cacheable_Outbuf(void *pHandle);
ExynosVideoErrorType MFC_Set_OperatingRate(void *pHandle, unsigned int framerate);
ExynosVideoErrorType MFC_Set_RealTimePriority(void *pHandle, unsigned int RealTimePriority);
ExynosVideoErrorType MFC_Get_RealTimePriority(void *pHandle);
ExynosVideoErrorType MFC_Disable_LazyUnmap(void *pHandle);

ExynosVideoErrorType MFC_Run_Inbuf(void *pHandle);
ExynosVideoErrorType MFC_Run_Outbuf(void *pHandle);
ExynosVideoErrorType MFC_Stop_Inbuf(void *pHandle);
ExynosVideoErrorType MFC_Stop_Outbuf(void *pHandle);

int MFC_Find_Inbuf(void *pHandle, void *pBuffer);
int MFC_FindEmptySlot_Inbuf(void *pHandle);
int MFC_FindEmptySlot_Outbuf(void *pHandle, bool bIsEncode);

ExynosVideoErrorType MFC_Cleanup_Buffer_Inbuf(void *pHandle);
ExynosVideoErrorType MFC_Cleanup_Buffer_Outbuf(void *pHandle);

ExynosVideoErrorType MFC_setupCodecOSALBuffer(CodecOSALVideoContext *pCtx, ExynosVideoBuffer *pVideoBuffer,
                                          ExynosVideoPadInfo *pSrcPad, CodecOSAL_Buffer *buf, bool isEncode);
ExynosVideoErrorType MFC_Dequeue_Inbuf(void *pHandle, ExynosVideoBuffer *pVideoBuffer, bool isEncode);

typedef ExynosVideoErrorType (*StateProcessing) (void *pHandle, ExynosVideoBuffer *pVideoBuffer,
                                        ExynosVideoPadInfo *pDstPad, ExynosVideoBuffer *pOutbuf, CodecOSAL_Buffer *buf);
ExynosVideoErrorType MFC_Common_Dequeue_Outbuf(void *pHandle, ExynosVideoBuffer *pVideoBuffer, StateProcessing func);

void MFC_ResourceClear(CodecOSALVideoContext *pCtx, void (*CodecSpecificClear)(CodecOSALVideoContext *pCtx));

#endif /* _EXYNOS_VIDEO_COMMON__API_H_ */

