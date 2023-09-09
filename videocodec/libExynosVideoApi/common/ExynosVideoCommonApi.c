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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <pthread.h>

#ifdef USE_USER_POLL_EVENT
#include <sys/eventfd.h>
#endif

#include "ExynosVideo_OSAL.h"
#include "ExynosVideoCommonApi.h"

// #define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosVideoCommon"

#include "ExynosVideo_OSAL_Log.h"

int MFC_QueryCap(CodecOSALVideoContext *pCtx) {
    int ret = -1;

    if (Codec_OSAL_QueryCap(pCtx) != 0) {
        ALOGE("%s: Failed to querycap", __FUNCTION__);
        return ret;
    }

    return 0;
}

int MFC_Pad_init(CodecOSALVideoContext *pCtx) {
    int ret = -1;
    int i;

    pthread_mutex_t *pMutex   = NULL;

    for (i = 0; i < VIDEO_MAX_PAD_NUM; i++) {
        pCtx->videoCtx.pad[i].bStreamOn = VIDEO_FALSE;

        /* mutex for input */
        pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        if (pMutex == NULL) {
            ALOGE("%s: Failed to allocate mutex", __FUNCTION__);
            return ret;
        }

        if (pthread_mutex_init(pMutex, NULL) != 0) {
            free(pMutex);
            return ret;
        }

        pCtx->videoCtx.pad[i].pMutex = (void*)pMutex;

        MFC_Create_Epoll(pCtx, i);
    }

    return 0;
}

int MFC_SharedMem_init(CodecOSALVideoContext *pCtx) {
    int ret = -1;
    int hIonClient = -1;

    /* for shared memory */
    hIonClient = exynos_ion_open();

    if (hIonClient < 0) {
        ALOGE("%s: Failed to create ion_client", __FUNCTION__);
        pCtx->videoCtx.hIONHandle = -1;
        return ret;
    }

    pCtx->videoCtx.hIONHandle = hIonClient;

    return 0;
}

int MFC_Init_Common(CodecOSALVideoContext *pCtx) {
    int ret = -1;

    if (MFC_QueryCap(pCtx) != 0) {
        return ret;
    }

    if (MFC_Pad_init(pCtx) != 0) {
        return ret;
    }

    if (MFC_SharedMem_init(pCtx) != 0) {
        return ret;
    }

    return 0;
}

/*
 * Create epoll
 */
int MFC_Create_Epoll(CodecOSALVideoContext *pCtx, int padIndex) {
    int ret = -1;

#ifdef USE_EPOLL
    pCtx->videoCtx.pad[padIndex].hPoll = Codec_OSAL_Epoll_Create();
    if (pCtx->videoCtx.pad[padIndex].hPoll == NULL) {
        ALOGE("%s: Failed to Codec_OSAL_Epoll_Create()", __FUNCTION__);
        return ret;
    } else {
        CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD];
        int cnt = 0;

        memset(pollfd, 0, sizeof(pollfd));

#ifdef USE_USER_POLL_EVENT
        if (pCtx->videoCtx.hUser != -1) {
            pollfd[cnt].fd = pCtx->videoCtx.hUser;
            pollfd[cnt].events = CODEC_OSAL_POLL_USR_EVENT | CODEC_OSAL_POLL_ERR_EVENT;
            cnt++;
        }
#endif
        pollfd[cnt].fd = pCtx->videoCtx.hDevice;
        pollfd[cnt].events = ((padIndex == VIDEO_INDEX_SRC_PAD)? CODEC_OSAL_POLL_SRC_EVENT:CODEC_OSAL_POLL_DST_EVENT);
        pollfd[cnt].events |= CODEC_OSAL_POLL_ERR_EVENT;
        cnt++;

        Codec_OSAL_Epoll_Regist(pCtx->videoCtx.pad[padIndex].hPoll, pollfd, cnt);
    }
#endif

    return 0;
}

#ifndef USE_EPOLL
/*
 * [OPS] Wait Buffer Common
 */
ExynosVideoErrorType MFC_Wait_Buffer_Common(
    void                *pHandle,
    ExynosVideoPollType *avail, bool bIsEncode, PortType port) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD];
    int nfd = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    short events = 0;
    ExynosVideoPollType pollType = POLL_TYPE_NONE;
    if (port == InputPort) {
        events = CODEC_OSAL_POLL_SRC_EVENT;
        pollType = POLL_TYPE_SRC;
    } else {
        events = CODEC_OSAL_POLL_DST_EVENT;
        pollType = POLL_TYPE_DST;
    }

    /* MFC device poll event */
    pollfd[0].fd        = pCtx->videoCtx.hDevice;
    pollfd[0].events    = (events | CODEC_OSAL_POLL_ERR_EVENT);
    pollfd[0].revents   = 0;
    nfd++;

    (*avail) = POLL_TYPE_NONE;

#ifdef USE_USER_POLL_EVENT
    /* user poll event */
    pollfd[1].fd        = pCtx->videoCtx.hUser;
    pollfd[1].events    = CODEC_OSAL_POLL_USR_EVENT | CODEC_OSAL_POLL_ERR_EVENT;
    pollfd[1].revents   = 0;
    nfd++;
#endif

    if (Codec_OSAL_Poll(pCtx, pollfd, nfd, CODEC_OSAL_POLL_TIMEOUT) > 0) {
#ifdef USE_USER_POLL_EVENT
        if (pollfd[1].revents & (CODEC_OSAL_POLL_USR_EVENT | CODEC_OSAL_POLL_ERR_EVENT)) {
            (*avail) |= POLL_TYPE_USER;
            goto EXIT;
        }
#endif
        if (pollfd[0].revents & events) {
            (*avail) |= pollType;
            goto EXIT;
        }

        if (pollfd[0].revents & CODEC_OSAL_POLL_ERR_EVENT) {
            ret = VIDEO_ERROR_POLL;
            goto EXIT;
        }

        if ((bIsEncode == true) &&
            ((*avail) == POLL_TYPE_NONE)) {
            ret = VIDEO_ERROR_POLL;
            goto EXIT;
        }
#if 1
        /* TODO : fix a problem awakened it by weird event */
        ret = VIDEO_ERROR_TRY_AGAIN;
        goto EXIT;
#endif
    } else {
        /* error or timeout state */
        ret = VIDEO_ERROR_POLL;
        goto EXIT;
    }

EXIT:
    ALOGV("%s: avail(%x), ret(%x), %x, %x", __FUNCTION__, *avail, ret, pollfd[0].revents, pollfd[1].revents);

    return ret;
}
#else
/*
 * [OPS] Wait Buffer Common // epoll version
 */
ExynosVideoErrorType MFC_Wait_Buffer_Common_Epoll(
    void                *pHandle,
    ExynosVideoPollType *avail, PortType port) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD];
    int pad = (port == InputPort)? VIDEO_INDEX_SRC_PAD: VIDEO_INDEX_DST_PAD;
    int events = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    (*avail) = POLL_TYPE_NONE;

    events = Codec_OSAL_Epoll(pCtx->videoCtx.pad[pad].hPoll, pollfd, CODEC_OSAL_POLL_TIMEOUT);
    if (events > 0) {
        int i = 0;

        for (i = 0; i < events; i++) {
#ifdef USE_USER_POLL_EVENT
            if ((pollfd[i].fd == pCtx->videoCtx.hUser) &&
                (pollfd[i].revents & (CODEC_OSAL_POLL_USR_EVENT | CODEC_OSAL_POLL_ERR_EVENT))) {
                (*avail) |= POLL_TYPE_USER;
                goto EXIT;
            }
#endif
            if (pollfd[i].fd == pCtx->videoCtx.hDevice) {
                if (pollfd[i].revents & CODEC_OSAL_POLL_SRC_EVENT) {
                    (*avail) |= POLL_TYPE_SRC;
                    goto EXIT;
                }

                if (pollfd[i].revents & CODEC_OSAL_POLL_DST_EVENT) {
                    (*avail) |= POLL_TYPE_DST;
                    goto EXIT;
                }

                if (pollfd[i].revents & CODEC_OSAL_POLL_ERR_EVENT) {
                    ret = VIDEO_ERROR_POLL;
                    goto EXIT;
                }
            }
        }
#if 1
        /* TODO : fix a problem awakened it by weird event */
        ret = VIDEO_ERROR_TRY_AGAIN;
        goto EXIT;
#endif
    } else {
        /* error or timeout state */
        ret = VIDEO_ERROR_POLL;
        goto EXIT;
    }

EXIT:
    ALOGV("%s: avail(%x), ret(%x)", __FUNCTION__, *avail, ret);

    return ret;
}
#endif

/*
 * [OPS] Stop Wait Buffer
 */
ExynosVideoErrorType MFC_Stop_Wait_Buffer(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

#ifdef USE_USER_POLL_EVENT
    {
        const uint64_t buf = 1;

        if (pCtx->videoCtx.hUser != -1) {
            if (write(pCtx->videoCtx.hUser, &buf, sizeof(buf)) == -1) {
                ALOGE("%s: Failed to raise user poll event", __FUNCTION__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            ALOGV("%s: raise user poll event", __FUNCTION__);
        }
    }
#endif

    ret = VIDEO_ERROR_NONE;

EXIT:
    return ret;
}

/*
 * [OPS] Reset Wait Buffer
 */
ExynosVideoErrorType MFC_Reset_Wait_Buffer(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

#ifdef USE_USER_POLL_EVENT
    {
        uint64_t buf;

        if (pCtx->videoCtx.hUser != -1) {
            if (read(pCtx->videoCtx.hUser, &buf, sizeof(buf)) == -1) {
                ALOGE("%s: Failed to clear user poll event", __FUNCTION__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            ALOGV("%s: clear user poll event", __FUNCTION__);
        }
    }
#endif

    ret = VIDEO_ERROR_NONE;

EXIT:
    return ret;
}

/*
 * [OPS] Get Real Time Priority
 */
ExynosVideoErrorType MFC_Get_RealTimePriority(void *pHandle) {
    CodecOSALVideoContext *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret   = VIDEO_ERROR_NONE;

    int value = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_ACTUAL_PRIORITY, &value) != 0) {
        ALOGE("%s: Failed to get real time priority", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (value == 0) {
        ret = VIDEO_ERROR_NONE;
    } else {
        ALOGE("%s: failed to get real time resource(value: %d)", __FUNCTION__, value);
        ret = VIDEO_ERROR_NOSUPPORT;
    }

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Enable Cacheable (Input)
 */
ExynosVideoErrorType MFC_Enable_Cacheable_Inbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_CACHEABLE_BUFFER, 2) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Enable Cacheable (Output)
 */
ExynosVideoErrorType MFC_Enable_Cacheable_Outbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_CACHEABLE_BUFFER, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_Set_OperatingRate(
    void        *pHandle,
    unsigned int framerate) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (framerate != 0xFFFFFFFF) {
        framerate = framerate * 1000;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ACTUAL_FRAMERATE, framerate) != 0) {
        ALOGE("%s: Failed to set operating rate", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_Set_RealTimePriority(
    void        *pHandle,
    unsigned int RealTimePriority) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ACTUAL_PRIORITY, RealTimePriority) != 0) {
        ALOGE("%s: Failed to set realtime priority", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_Disable_LazyUnmap(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_SKIP_LAZY_UNMAP, 1) != 0) {
        ALOGE("%s: Failed to disable lazy unmap", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Run (Input)
 */
ExynosVideoErrorType MFC_Run_Inbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    if (pSrcPad->bStreamOn == VIDEO_FALSE) {
        if (Codec_OSAL_SetStreamOn(pCtx, CODEC_OSAL_BUF_TYPE_SRC) != 0) {
            ALOGE("%s: Failed to streamon for input buffer", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pSrcPad->bStreamOn = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Run (Output)
 */
ExynosVideoErrorType MFC_Run_Outbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    if (pDstPad->bStreamOn == VIDEO_FALSE) {
        if (Codec_OSAL_SetStreamOn(pCtx, CODEC_OSAL_BUF_TYPE_DST) != 0) {
            ALOGE("%s: Failed to streamon for output buffer", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pDstPad->bStreamOn = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Stop (Input)
 */
ExynosVideoErrorType MFC_Stop_Inbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    int i = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    if (pSrcPad->bStreamOn == VIDEO_TRUE) {
        if (Codec_OSAL_SetStreamOff(pCtx, CODEC_OSAL_BUF_TYPE_SRC) != 0) {
            ALOGE("%s: Failed to streamoff for input buffer", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pSrcPad->bStreamOn = VIDEO_FALSE;
    }

    memset(&(pSrcPad->slot), 0, sizeof(pSrcPad->slot));

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Stop (Output)
 */
ExynosVideoErrorType MFC_Stop_Outbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    int i = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    if (pDstPad->bStreamOn == VIDEO_TRUE) {
        if (Codec_OSAL_SetStreamOff(pCtx, CODEC_OSAL_BUF_TYPE_DST) != 0) {
            ALOGE("%s: Failed to streamoff for output buffer", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pDstPad->bStreamOn = VIDEO_FALSE;
    }

    memset(&(pDstPad->slot), 0, sizeof(pDstPad->slot));

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Find (Input)
 */
int MFC_Find_Inbuf(
    void *pHandle,
    void *pBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;

    int nIndex = -1, i;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    for (i = 0; i < pSrcPad->nBufNum; i++) {
        if (pSrcPad->slot[i].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pSrcPad->slot[i].buffer.planes[0].addr == pBuffer)) {
                nIndex = i;
                break;
            }
        }
    }

EXIT:
    return nIndex;
}

/*
 * [Buffer OPS] Find index (Input)
 */
int MFC_FindEmptySlot_Inbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;

    int nIndex = -1, i;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    for (i = 0; i < pSrcPad->nBufNum; i++) {
        if ((pSrcPad->slot[i].bQueued == VIDEO_FALSE) &&
            (pSrcPad->slot[i].bSlotUsed == VIDEO_FALSE)) {
            nIndex = i;
            break;
        }
    }

    if (nIndex == -1) {
        for (i = 0; i < pSrcPad->nBufNum; i++) {
            if (pSrcPad->slot[i].bQueued == VIDEO_FALSE) {
                nIndex = i;
                break;
            }
        }
    }

EXIT:
    return nIndex;
}

/*
 * [Buffer OPS] Find index on non-used slot (Output)
 */
int MFC_FindEmptySlot_Outbuf(void *pHandle, bool bIsEncode) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;

    int nIndex = -1, i;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    for (i = 0; i < pDstPad->nBufNum; i++) {
        bool bCheck = (bIsEncode == true) ? true : (pDstPad->slot[i].bSlotUsed == VIDEO_FALSE);
        if ((pDstPad->slot[i].bQueued == VIDEO_FALSE) && bCheck) {
            nIndex = i;
            break;
        }
    }

EXIT:
    return nIndex;
}

/*
 * [Buffer OPS] Cleanup Buffer (Input)
 */
ExynosVideoErrorType MFC_Cleanup_Buffer_Inbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_ReqBuf reqBuf;
    int nBufferCount = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = (ExynosVideoPadInfo *)&(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    nBufferCount = 0; /* for clean-up */

    memset(&reqBuf, 0, sizeof(reqBuf));

    reqBuf.type = CODEC_OSAL_BUF_TYPE_SRC;
    reqBuf.count = nBufferCount;
    reqBuf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&(pSrcPad->slot), 0, sizeof(pSrcPad->slot));
    pSrcPad->nBufNum = (int)reqBuf.count;

EXIT:
    return ret;
}

/*
 * [Buffer OPS] Cleanup Buffer (Output)
 */
ExynosVideoErrorType MFC_Cleanup_Buffer_Outbuf(void *pHandle) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_ReqBuf reqBuf;
    int nBufferCount = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = (ExynosVideoPadInfo *)&(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    nBufferCount = 0; /* for clean-up */

    memset(&reqBuf, 0, sizeof(reqBuf));

    reqBuf.type = CODEC_OSAL_BUF_TYPE_DST;
    reqBuf.count = nBufferCount;
    reqBuf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&(pDstPad->slot), 0, sizeof(pDstPad->slot));
    pDstPad->nBufNum = (int)reqBuf.count;

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_setupCodecOSALBuffer(CodecOSALVideoContext *pCtx, ExynosVideoBuffer *pVideoBuffer,
                                          ExynosVideoPadInfo *pSrcPad, CodecOSAL_Buffer *buf, bool isEncode) {
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;
    int index, i;

    memset(buf, 0, sizeof(CodecOSAL_Buffer));
    buf->type    = CODEC_OSAL_BUF_TYPE_SRC;
    buf->nPlane  = pVideoBuffer->nPlaneCnt;

    index = MFC_Find_Inbuf(pCtx, pVideoBuffer->planes[0].addr);
    if (index == -1) {
        ALOGV("%s: Failed to find index", __FUNCTION__);
        index = MFC_FindEmptySlot_Inbuf(pCtx);
        if (index == -1) {
            ALOGE("%s: Failed to get index", __FUNCTION__);
            ret = VIDEO_ERROR_NOBUFFERS;
            goto EXIT;
        }
    }

    buf->index = index;
    ALOGV("%s: pSrcPad->slot[%d].bQueued:%d, pFd[0]:%d",
           __FUNCTION__, index, pSrcPad->slot[buf->index].bQueued, pVideoBuffer->planes[0].fd);

    buf->memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);
    for (i = 0; i < buf->nPlane; i++) {
        if (pCtx->videoCtx.instInfo.nMemoryType == VIDEO_MEMORY_DMABUF)
            buf->planes[i].addr = (void *)(unsigned long)pVideoBuffer->planes[i].fd;
        else
            buf->planes[i].addr = pVideoBuffer->planes[i].addr;

        buf->planes[i].bufferSize    = pVideoBuffer->planes[i].allocSize;
        buf->planes[i].dataLen       = ((isEncode == true) ? pVideoBuffer->planes[i].allocSize : /* for exact cache flush size */
                                                             pVideoBuffer->planes[i].dataSize);

        ALOGV("%s: pSrcPad->slot[%d].buffer[%d]/ fd=%d, alloc=%d, len=%d", __FUNCTION__, index, i,
                    pVideoBuffer->planes[i].fd, pVideoBuffer->planes[i].allocSize, pVideoBuffer->planes[i].dataSize);
    }

    if (pVideoBuffer->planes[0].dataSize == 0) {
        buf->flags = CODEC_OSAL_FLAG_EMPTY_DATA | CODEC_OSAL_FLAG_EOS;
        ALOGD("%s: EMPTY DATA", __FUNCTION__);
    } else {
        buf->flags = CODEC_OSAL_FLAG_UNUSED;

        if (pVideoBuffer->extraInfo.nFlags & VIDEO_FLAG_CSD) {
            buf->flags |= CODEC_OSAL_FLAG_CSD;
        }

        if (pVideoBuffer->extraInfo.nFlags & VIDEO_FLAG_EMPTY_DATA) {
            buf->flags |= CODEC_OSAL_FLAG_EMPTY_DATA;
        }

        if (pVideoBuffer->extraInfo.nFlags & VIDEO_FLAG_EOS) {
            buf->flags |= CODEC_OSAL_FLAG_EOS;
        }

        if (pVideoBuffer->extraInfo.nFlags & VIDEO_FLAG_REPLICA_DATA) {
            buf->flags |= CODEC_OSAL_FLAG_REPLICA_DATA;
        }

        if (isEncode == true) {
            if ((pCtx->videoCtx.specificInfo.enc.actualFormat != VIDEO_COLORFORMAT_UNKNOWN) &&
                (pCtx->videoCtx.specificInfo.enc.actualFormat < VIDEO_COLORFORMAT_SBWC_START)) {
                buf->flags |= CODEC_OSAL_FLAG_UNCOMP_FORMAT;
                pCtx->videoCtx.specificInfo.enc.actualFormat = VIDEO_COLORFORMAT_UNKNOWN;
                ALOGV("%s: SBWC is enabled. but, normal format", __FUNCTION__);
            }

            if (pCtx->videoCtx.specificInfo.enc.bEnableGDCvOTF == VIDEO_TRUE) {
                if (pCtx->videoCtx.specificInfo.enc.bUseGDCvOTF == VIDEO_TRUE) {
                    buf->flags |= CODEC_OSAL_FLAG_GDC_OTF;
                } else {
                    ALOGD("%s: GDCvOTF is enabled. but, normal frame", __FUNCTION__);
                }
            }
        }

        if (buf->flags & (CODEC_OSAL_FLAG_CSD | CODEC_OSAL_FLAG_EOS))
            ALOGD("%s: DATA with flags(0x%x)", __FUNCTION__, buf->flags);
    }

    signed long long sec  = pVideoBuffer->extraInfo.nTimeStamp / 1E6;
    signed long long usec = pVideoBuffer->extraInfo.nTimeStamp - (sec * 1E6);
    buf->timestamp.tv_sec  = (long)sec;
    buf->timestamp.tv_usec = (long)usec;

    memcpy(&(pSrcPad->slot[buf->index].buffer), pVideoBuffer, sizeof(ExynosVideoBuffer));

    pSrcPad->slot[buf->index].bQueued = VIDEO_TRUE;
    pSrcPad->slot[buf->index].bSlotUsed = VIDEO_TRUE;

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_Dequeue_Inbuf(void *pHandle, ExynosVideoBuffer *pVideoBuffer, bool isEncode) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    CodecOSAL_Buffer buf;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pVideoBuffer) == false)) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    if (pSrcPad->bStreamOn == VIDEO_FALSE) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = CODEC_OSAL_BUF_TYPE_SRC;
    buf.nPlane = pSrcPad->nPlane;
    buf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_DequeueBuf(pCtx, &buf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if ((isEncode == true) &&
        (buf.index < 0)) {
        ALOGE("[%s] tag is (%d)", __FUNCTION__, buf.index);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pSrcPad->pMutex;
    pthread_mutex_lock(pMutex);

    if (pSrcPad->slot[buf.index].bQueued == VIDEO_TRUE) {
        memcpy(pVideoBuffer, &(pSrcPad->slot[buf.index].buffer), sizeof(ExynosVideoBuffer));

        if (!isEncode) {
            pVideoBuffer->extraInfo.specific.dec.frameType = buf.frameType;
        } else {
            pVideoBuffer->extraInfo.specific.enc.frameType = buf.frameType;
        }
    } else {
        ret = VIDEO_ERROR_NOBUFFERS;
    }

    pSrcPad->slot[buf.index].bQueued = VIDEO_FALSE;
    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}

ExynosVideoErrorType MFC_Common_Dequeue_Outbuf(void *pHandle, ExynosVideoBuffer *pVideoBuffer, StateProcessing func) {
    CodecOSALVideoContext  *pCtx    = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo     *pDstPad = NULL;
    ExynosVideoErrorType    ret     = VIDEO_ERROR_NONE;
    pthread_mutex_t        *pMutex  = NULL;
    ExynosVideoBuffer      *pOutbuf = NULL;

    CodecOSAL_Buffer buf;

    int i;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pVideoBuffer) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = (ExynosVideoPadInfo *)&(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    if (pDstPad->bStreamOn == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type    = CODEC_OSAL_BUF_TYPE_DST;
    buf.nPlane  = pDstPad->nPlane;
    buf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    /* encode case: returning DQBUF_EIO means MFC H/W status is invalid */
    /* decode case: HACK: pOutbuf return -1 means DECODING_ONLY for almost cases */
    if (Codec_OSAL_DequeueBuf(pCtx, &buf) != 0) {
        if (errno == EIO)
            ret = VIDEO_ERROR_DQBUF_EIO;
        else
            ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pDstPad->pMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &(pDstPad->slot[buf.index].buffer);
    if (pDstPad->slot[buf.index].bQueued == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_NOBUFFERS;
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    for (i = 0; i < buf.nPlane; i++)
        pOutbuf->planes[i].dataSize = buf.planes[i].dataLen;

    func(pHandle, pVideoBuffer, pDstPad, pOutbuf, &buf);

    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}


void MFC_ResourceClear(CodecOSALVideoContext *pCtx, void (*CodecSpecificClear)(CodecOSALVideoContext *pCtx)) {
    int i = 0;

    for (i = 0; i < VIDEO_MAX_PAD_NUM; i++) {
        if (pCtx->videoCtx.pad[i].pMutex != NULL) {
            pthread_mutex_destroy(pCtx->videoCtx.pad[i].pMutex);
            free(pCtx->videoCtx.pad[i].pMutex);
        }
#ifdef USE_EPOLL
        if (pCtx->videoCtx.pad[i].hPoll != NULL) {
            Codec_OSAL_Epoll_Destroy(pCtx->videoCtx.pad[i].hPoll);
            pCtx->videoCtx.pad[i].hPoll = NULL;
        }
#endif
    }

    CodecSpecificClear(pCtx);

    /* free a ion_client */
    if (pCtx->videoCtx.hIONHandle >= 0) {
        exynos_ion_close(pCtx->videoCtx.hIONHandle);
        pCtx->videoCtx.hIONHandle = -1;
    }

#ifdef USE_USER_POLL_EVENT
    if (pCtx->videoCtx.hUser >= 0) {
        close(pCtx->videoCtx.hUser);
        pCtx->videoCtx.hUser = -1;
    }
#endif

    Codec_OSAL_DevClose(pCtx);
}

