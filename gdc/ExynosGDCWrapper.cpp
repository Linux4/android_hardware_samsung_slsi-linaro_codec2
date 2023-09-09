/*
 *
 * Copyright 2018 Samsung Electronics S.LSI Co. LTD
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
#include "ExynosGDCWrapper.h"
#include "ExynosGDCInterface.h"

#include "ExynosThreadPool.h"
#include "ExynosQueue.h"

#include "VendorVideoAPI.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosGDCWrapper"

#define MAX_TAG_NUM GDC_V4L2_MAX_BUF_COUNT


class ExynosGDCWrapper::GDCImpl : public ExynosLog, public std::enable_shared_from_this<ExynosGDCWrapper::GDCImpl> {
public:
    GDCImpl(std::string name) : ExynosLog(name + "-Impl") {
        mIntf = nullptr;
        mDequeueThread = std::make_shared<ExynosThreadPool>(1, mObjName + "-Dequeue");
        mIsConfigured = false;
        mIsStarted = false;
        mFrameIndex = 0;
        mbLogOff = false;
        mGDCMode = EXYNOS_GDC_MFC_CONNECTTION_NONE;
    }

    ~GDCImpl() {
        if ((mIntf.get() != nullptr) &&
            (mIsConfigured)) {
            flush();
            destroy();

            mIsConfigured = false;
        }

        if (mDequeueThread.get() != nullptr) {
            mDequeueThread->stop();
            mDequeueThread.reset();
        }

        mIntf.reset();
    }

    bool run(ExynosBufferInfo &input, ExynosBufferInfo &output);
    bool flush();

private:
    /* function for thread pool owned by self */
    bool doDequeue();

    /* add function for GDCImpl */
    bool setup(ExynosBufferInfo &input, ExynosBufferInfo &output);
    bool create();
    void destroy();
    bool srcSetup(ExynosBufferInfo &input);
    bool dstSetup(ExynosBufferInfo &output);
    bool enqueue(ExynosBufferInfo &input, ExynosBufferInfo &output);
    bool srcEnqueue(ExynosBufferInfo &input, uint32_t tag, GDCInfo &info);
    bool dstEnqueue(ExynosBufferInfo &output, uint32_t tag, GDCInfo &info);
    bool requestDequeue();
    bool dequeue();


    std::shared_ptr<ExynosGDCInterface> mIntf;
    std::shared_ptr<ExynosThreadPool> mDequeueThread;

    std::mutex mMutex;
    bool mIsConfigured;
    bool mIsStarted;
    enum ExynosGDCConnection mGDCMode;

    uint32_t mFrameIndex;
    using GDCOutBufInfo = std::pair<uint32_t, std::shared_ptr<ExynosBuffer>>;
    ExynosQueue<GDCOutBufInfo> mQueue;

    /* TOSO : buffer pool */

    GDCImpl() = delete;
};

bool ExynosGDCWrapper::GDCImpl::run(
    ExynosBufferInfo &input,
    ExynosBufferInfo &output) {
    ExynosLogFunctionTrace();

    if ((input.obj.get() == nullptr) ||
        (output.obj.get() == nullptr)) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    if (!mIsConfigured) {
        if (!setup(input, output)) {
            ExynosLogE("[%s] setup() is failed", __FUNCTION__);
            return false;
        }
    }

    if (!enqueue(input, output)) {
        ExynosLogE("[%s] enqueue() is failed", __FUNCTION__);
        return false;
    }

    return requestDequeue();
}

bool ExynosGDCWrapper::GDCImpl::flush() {
    ExynosLogFunctionTrace();

    if ((mIntf.get() != nullptr) &&
        (mIsStarted)) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mIntf->stop() != NO_ERROR) {
            ExynosLogE("[%s] stop() is failed", __FUNCTION__);
            return false;
        }

        mIsStarted = false;
        mQueue.clear();
    }

    auto shDequeueThread = mDequeueThread;
    if (shDequeueThread.get() != nullptr) {
        shDequeueThread->flush();

        auto syncFunc = [&]()->bool {
                            StaticExynosLog(Level::Debug, "GDCImpl::doFlushSync", "[doFlushSync] sync");
                            return true;
                        };

        auto err = shDequeueThread->post(std::string("GDCImpl::doFlushSync"), std::move(syncFunc));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doFlushSync() is timed out", __FUNCTION__);
        }
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::doDequeue() {
    ExynosLogFunctionTrace();

    if (!mIsConfigured) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mIsStarted) {
            ExynosLogD("[%s] skip to dequeue", __FUNCTION__);
            return true;
        }
    }

    if (mIntf.get() != nullptr) {
        return dequeue();
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::setup(
    ExynosBufferInfo &input,
    ExynosBufferInfo &output) {
    ExynosLogFunctionTrace();

    if (!mIsConfigured) {
        {
            auto metadata = (ExynosVideoMeta *)input.obj->metadata();
            if ((metadata == nullptr) ||
                (!(metadata->eType & VIDEO_INFO_TYPE_GDC_OTF))) {
                ExynosLogE("[%s] metadata is invalid", __FUNCTION__);
                return false;
            }

            switch (metadata->data.enc.nUseGdcOTF) {
            case GDC_TYPE_VOTF:
                mGDCMode = EXYNOS_GDC_MFC_CONNECTTION_VIRTUAL_OTF;
                break;
            case GDC_TYPE_OTF:
                mGDCMode = EXYNOS_GDC_MFC_CONNECTTION_OTF;
                break;
            default:
                mGDCMode = EXYNOS_GDC_MFC_CONNECTTION_M2M;
                break;
            }

            ExynosLogD("[%s] GDC mode is 0x%x // HWConnection(0x%x)", __FUNCTION__, metadata->data.enc.nUseGdcOTF, mGDCMode);
        }

        if (!create()) {
            return false;
        }

        if (!srcSetup(input)) {
            destroy();
            return false;
        }

        if (!dstSetup(output)) {
            destroy();
            return false;
        }

        auto err = mIntf->init();
        if (err != NO_ERROR) {
            ExynosLogE("[%s] init() is failed", __FUNCTION__);
            return false;
        }

        mIsConfigured = true;
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::create() {
    ExynosLogFunctionTrace();

    if (mIntf.get() != nullptr) {
        return true;
    }

    mIntf = std::make_shared<ExynosGDCInterface>();

    auto err = mIntf->create();
    if (err != NO_ERROR) {
        ExynosLogE("[%s] create() is failed", __FUNCTION__);
        mIntf.reset();
        return false;
    }

    return true;
}

void ExynosGDCWrapper::GDCImpl::destroy() {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return;
    }

    auto err = mIntf->destroy();
    if (err != NO_ERROR) {
        ExynosLogE("[%s] destroy() is failed", __FUNCTION__);
        return;
    }

    return;
}

bool ExynosGDCWrapper::GDCImpl::srcSetup(ExynosBufferInfo &input) {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return false;
    }

    auto metadata = input.obj->metadata();
    if (metadata == nullptr) {
        ExynosLogE("[%s] metadata is invalid", __FUNCTION__);
        return false;
    }

    GDCInfo &info = metadata->data.enc.sGDCInfo;

    auto err = mIntf->setSrcImageSize(info.cropInfo.fullW, info.cropInfo.fullH);
    if (err != NO_ERROR) {
        ExynosLogE("[%s] setSrcImageSize() is failed", __FUNCTION__);
        return false;
    }

    err = mIntf->setSrcColorFormat(input.stImageInfo.nFormat, input.nPlane);
    if (err != NO_ERROR) {
        ExynosLogE("[%s] setSrcColorFormat() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::dstSetup(ExynosBufferInfo &output) {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return false;
    }

    auto err = mIntf->setDstImageSize(output.stImageInfo.nStride, output.stImageInfo.nHeight);
    if (err != NO_ERROR) {
        ExynosLogE("[%s] setDstImageSize() is failed", __FUNCTION__);
        return false;
    }

    err = mIntf->setDstColorFormat(output.stImageInfo.nFormat, output.nPlane);
    if (err != NO_ERROR) {
        ExynosLogE("[%s] setDstColorFormat() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::enqueue(
    ExynosBufferInfo &input,
    ExynosBufferInfo &output) {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return false;
    }

    auto metadata = input.obj->metadata();
    if (metadata == nullptr) {
        ExynosLogE("[%s] metadata is invalid", __FUNCTION__);
        return false;
    }

    /* check flag in metadata */
    GDCInfo &info = metadata->data.enc.sGDCInfo;

    /* grid table */
    if (mIntf->setGridTable((int32_t *)info.gridTable.gridX, (int32_t *)info.gridTable.gridY,
                            info.gridTable.width, info.gridTable.height) != NO_ERROR) {
        ExynosLogE("[%s] pollLast() is failed", __FUNCTION__);
        return false;
    }

    /* set information */
    auto tag = mFrameIndex;
    mFrameIndex = ((mFrameIndex + 1) % MAX_TAG_NUM);

    if (!srcEnqueue(input, tag, info)) {
        return false;
    }

    if (!dstEnqueue(output, tag, info)) {
        return false;
    }

    /* stream on */
    if (!mIsStarted) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mIntf->start() != NO_ERROR) {
            ExynosLogE("[%s] start() is failed", __FUNCTION__);
            return false;
        }

        mIsStarted = true;
    }

    /* run */
    if (mIntf->pollLast() != NO_ERROR) {
        ExynosLogE("[%s] pollLast() is failed", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] index:%d, fd:%d", __FUNCTION__, tag, (output.obj->handle())->data[0]);
    mQueue.enqueue(GDCOutBufInfo(tag, output.obj));

    return true;
}

bool ExynosGDCWrapper::GDCImpl::srcEnqueue(
    ExynosBufferInfo &input,
    uint32_t          tag,
    GDCInfo          &info) {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return false;
    }

    {
        struct ExynosGDCBuf buf;

        buf.index = tag;
        buf.planeCount = input.nPlane;
        for (int i = 0; i < input.nPlane; i++) {
            buf.planes[i].fd        = input.nFD[i];
            buf.planes[i].length    = input.nAllocLen[i];
        }

        buf.HWConnection = mGDCMode;
        buf.gridMode     = info.is_grid_mode;
        buf.bypassMode   = info.is_bypass_mode;

        if (mIntf->setInputBuffer(buf) != NO_ERROR) {
            ExynosLogE("[%s] setInputBuffer() is failed", __FUNCTION__);
            return false;
        }
    }

    {
        struct ExynosGDCSizeParam size;

        size.cropX = info.cropInfo.rect.x;
        size.cropY = info.cropInfo.rect.y;
        size.cropW = info.cropInfo.rect.w;
        size.cropH = info.cropInfo.rect.h;
        // size.fullW = info.cropInfo.fullW;
        // size.fullH = info.cropInfo.fullH;

        if (mIntf->setInputSize(size) != NO_ERROR) {
            ExynosLogE("[%s] setInputSize() is failed", __FUNCTION__);
            return false;
        }
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::dstEnqueue(
    ExynosBufferInfo &output,
    uint32_t          tag,
    GDCInfo          &info) {
    ExynosLogFunctionTrace();

    if (mIntf.get() == nullptr) {
        return false;
    }

    {
        struct ExynosGDCBuf buf;

        buf.index = tag;
        buf.planeCount = output.nPlane;
        for (int i = 0; i < output.nPlane; i++) {
            buf.planes[i].fd        = output.nFD[i];
            buf.planes[i].length    = output.nAllocLen[i];
        }

        buf.HWConnection = mGDCMode;
        buf.gridMode     = info.is_grid_mode;
        buf.bypassMode   = info.is_bypass_mode;

        if (mIntf->setOutputBuffer(buf) != NO_ERROR) {
            ExynosLogE("[%s] setOutputBuffer() is failed", __FUNCTION__);
            return false;
        }
    }

    {
        struct ExynosGDCSizeParam size;

        size.cropX = 0;
        size.cropY = 0;
        size.cropW = output.stImageInfo.nStride;
        size.cropH = output.stImageInfo.nHeight;
        // size.fullW = output.stImageInfo.nStride;
        // size.fullH = output.stImageInfo.nHeight;

        if (mIntf->setOutputSize(size) != NO_ERROR) {
            ExynosLogE("[%s] setInputSize() is failed", __FUNCTION__);
            return false;
        }
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::requestDequeue() {
    ExynosLogFunctionTrace();

    auto shDequeueThread = mDequeueThread;
    if (shDequeueThread.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return true;
    }

    std::weak_ptr<GDCImpl> wkGDCImpl = std::static_pointer_cast<GDCImpl>(shared_from_this());
    auto err = shDequeueThread->post(std::string("GDCImpl::doDequeue"),
                                     weak_pointer_bind(false, &GDCImpl::doDequeue, wkGDCImpl));

    if (mGDCMode == EXYNOS_GDC_MFC_CONNECTTION_M2M) {
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doDequeue() is timed out", __FUNCTION__);
        }
    }

    return true;
}

bool ExynosGDCWrapper::GDCImpl::dequeue() {
    ExynosLogFunctionTrace();

    uint32_t index = 0;
    if (mIntf->pollFirst(index) != NO_ERROR) {
        ExynosLogD("[%s] pollFirst() is failed", __FUNCTION__);
        return true;
    }

    auto condFunc = [index](GDCOutBufInfo &e)->bool {
                        return (e.first == index)? true:false;
                    };

    GDCOutBufInfo info;
    if (!mQueue.dequeue(info, std::move(condFunc))) {
        ExynosLogD("[%s] dequeue(%d) is failed", __FUNCTION__, index);
        return true;
    }

    auto buffer = info.second;
    ExynosLogD("[%s] index:%d, fd:%d", __FUNCTION__, index, (buffer->handle())->data[0]);

    return true;
}

bool ExynosGDCWrapper::process(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    if (mImpl.get() == nullptr) {
        mImpl = std::make_shared<GDCImpl>(mObjName);
    }

    return mImpl->run(input, output);
}

bool ExynosGDCWrapper::flush() {
    ExynosLogFunctionTrace();

    if (mImpl.get() != nullptr) {
        mImpl->flush();
    }

    return true;
}
