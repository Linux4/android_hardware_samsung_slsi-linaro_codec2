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
#include <chrono>
#include <C2AllocatorGralloc.h>

#include "ExynosGraphicBuffer.h"

#include "exynos_format.h"
#include "ExynosBufferAllocator.h"
#include "C2ExynosSupport.h"
#include "C2ExynosParam.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosBufferAllocator"

using namespace android;
using namespace std::chrono_literals;
using namespace ::vendor::graphics;

#define WAIT_ALLOC_TIME 10ms

class ExynosBuffer::ExynosBufferOrigin {
public:
    ExynosBufferOrigin(std::shared_ptr<C2LinearBlock> block) {
        mLinearBlock = block;
        mGraphicBlock.reset();
    }

    ExynosBufferOrigin(std::shared_ptr<C2GraphicBlock> block) {
        mLinearBlock.reset();
        mGraphicBlock = block;
    }

    ~ExynosBufferOrigin() {
        mLinearBlock.reset();
        mGraphicBlock.reset();
    }

private:
    std::shared_ptr<C2LinearBlock> mLinearBlock;
    std::shared_ptr<C2GraphicBlock> mGraphicBlock;

    ExynosBufferOrigin() = delete;
};

class ExynosBufferImpl : public ExynosBuffer {
public:
    enum class Type : int {
        INVALID,
        ALLOC,   /* from C2BlockPool */
        IMPORT,  /* from C2Buffer */
    };

    ExynosBufferImpl(std::shared_ptr<C2LinearBlock> c2block, uint32_t size) {
        if (!CHECK_SHARED_PTR(c2block)) {
            /* obj is released */
            mType = Type::INVALID;
            return;
        }

        mType = Type::ALLOC;

        mBlock.id = C2BlockPool::BASIC_LINEAR;
        mBlock.c2block = c2block;

        mHandle = const_cast<ExynosBufferHandle *>(c2block->handle());

        mDataType   = LINEAR;
        mSize       = size;
        mDataLen    = 0;        /* empty */
        mWidth      = 0;
        mHeight     = 0;
        mFormat     = 0;
        mBuffer     = nullptr;

        memset((char *)&(mImageInfo), 0, sizeof(mImageInfo));
        mParams = nullptr;
    }

    ExynosBufferImpl(std::shared_ptr<C2GraphicBlock> c2block, uint32_t width, uint32_t height, uint32_t format) {
        if (!CHECK_SHARED_PTR(c2block)) {
            /* obj is released */
            mType = Type::INVALID;
            return;
        }

        mType = Type::ALLOC;

        mBlock.id = C2BlockPool::BASIC_GRAPHIC;
        mBlock.c2block = c2block;

        mHandle = const_cast<ExynosBufferHandle *>(c2block->handle());

        mDataType   = GRAPHIC;
        mSize       = 0;
        mDataLen    = 0;
        mWidth      = width;
        mHeight     = height;
        mFormat     = format;
        mBuffer     = nullptr;
        mParams     = nullptr;

        setGrallocMetadata(mHandle, true);
    }

    ExynosBufferImpl(std::shared_ptr<C2Buffer> c2buffer, ExynosBufferHandle *handle, uint32_t capacity) {
        if ((c2buffer.get() == nullptr) ||
            (handle == nullptr)) {
            /* invalid parameter */
            mType = Type::INVALID;
            return;
        }

        const C2BufferData data = c2buffer->data();
        C2ConstLinearBlock c2block = data.linearBlocks().front();

        mType = Type::IMPORT;

        mHandle = handle;

        mDataType   = LINEAR;
        mSize       = capacity;
        mDataLen    = c2block.size(); /* filled */
        mWidth      = 0;
        mHeight     = 0;
        mFormat     = 0;
        mBuffer     = c2buffer;

        memset((char *)&(mImageInfo), 0, sizeof(mImageInfo));
        mParams = nullptr;
    }

    ExynosBufferImpl(std::shared_ptr<C2Buffer> c2buffer, ExynosBufferHandle *handle, uint32_t width, uint32_t height) {
        if ((c2buffer.get() == nullptr) ||
            (handle == nullptr)) {
            /* invalid parameter */
            mType = Type::INVALID;
            return;
        }

        mType = Type::IMPORT;

        mHandle = handle;

        mDataType   = GRAPHIC;
        mSize       = 0;
        mDataLen    = 0;
        mWidth      = width;
        mHeight     = height;
        mBuffer     = c2buffer;
        mParams     = nullptr;

        setGrallocMetadata(mHandle);
    }

    virtual ~ExynosBufferImpl() {
        reset();
    }

    std::shared_ptr<C2Buffer> getC2Buffer() {
        if ((mType == Type::ALLOC) &&
            (mBuffer.get() == nullptr)) {
            switch (mBlock.id) {
            case C2BlockPool::BASIC_LINEAR:
            {
                auto c2block = std::get<std::shared_ptr<C2LinearBlock>>(mBlock.c2block);

                mBuffer = C2Buffer::CreateLinearBuffer(c2block->share(c2block->offset(), mDataLen, ::C2Fence()));
            }
                break;
            case C2BlockPool::BASIC_GRAPHIC:
            {
                auto c2block = std::get<std::shared_ptr<C2GraphicBlock>>(mBlock.c2block);

                C2Rect crop(mImageInfo.stCropInfo.nWidth, mImageInfo.stCropInfo.nHeight);

                StaticExynosLog(Level::Essential, "getC2Buffer", "[%s] gralloc crop : width(%d), height(%d), left(%d), top(%d)",
                                __FUNCTION__, mImageInfo.stCropInfo.nWidth, mImageInfo.stCropInfo.nHeight, mImageInfo.stCropInfo.nLeft, mImageInfo.stCropInfo.nTop);

                mBuffer = C2Buffer::CreateGraphicBuffer(c2block->share(crop.at(mImageInfo.stCropInfo.nLeft, mImageInfo.stCropInfo.nTop), ::C2Fence()));
            }
                break;
            default:
                /* TODO : error handling */
                break;
            }
        }

        return mBuffer;
    }

    void reset() {
        if (mType == Type::ALLOC) {
            switch (mBlock.id) {
            case C2BlockPool::BASIC_LINEAR:
            {
                auto c2block = std::get<std::shared_ptr<C2LinearBlock>>(mBlock.c2block);
                c2block.reset();
                mBlock.c2block = c2block;
            }
                break;
            case C2BlockPool::BASIC_GRAPHIC:
            {
                auto c2block = std::get<std::shared_ptr<C2GraphicBlock>>(mBlock.c2block);
                c2block.reset();
                mBlock.c2block = c2block;
            }
                break;
            default:
                /* TODO : error handling */
                break;
            }
        }

        mType       = Type::INVALID;
        mHandle     = nullptr;
        mSize       = 0;
        mWidth      = 0;
        mHeight     = 0;
        mFormat     = 0;
        mBuffer.reset();

        memset((char *)&(mImageInfo), 0, sizeof(mImageInfo));
        mParams = nullptr;
    }

    std::shared_ptr<ExynosBufferOrigin> origin() override {
        std::shared_ptr<ExynosBufferOrigin> ret = nullptr;

        switch (mBlock.id) {
        case C2BlockPool::BASIC_LINEAR:
        {
            auto c2block = std::get<std::shared_ptr<C2LinearBlock>>(mBlock.c2block);

            ret = std::make_shared<ExynosBufferOrigin>(c2block);
        }
            break;
        case C2BlockPool::BASIC_GRAPHIC:
        {
            auto c2block = std::get<std::shared_ptr<C2GraphicBlock>>(mBlock.c2block);

            ret = std::make_shared<ExynosBufferOrigin>(c2block);
        }
            break;
        default:
            /* TODO : error handling */
            break;
        }

        return ret;
    }

    bool destroy(uint32_t val) override {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mNotify.get() != nullptr) {
            if (mBuffer.get() != nullptr) {
                mBuffer.reset();
                mHandle = nullptr;
            }

            auto ret = (*mNotify)(val);
            mNotify.reset();

            return ret;
        }

        return true;
    }

private:
    struct C2Block {
        C2BlockPool::local_id_t id;
        std::variant<std::shared_ptr<C2LinearBlock>, std::shared_ptr<C2GraphicBlock>> c2block;
    };

    void setGrallocMetadata(const ExynosBufferHandle *const handle, bool clear = false) {
        uint32_t meta_width, meta_height, meta_format, meta_stride, meta_generation, meta_igbp_slot;
        uint64_t meta_usage, meta_igbp_id;

        android::_UnwrapNativeCodec2GrallocMetadata((const C2Handle *const)handle,
            &meta_width, &meta_height, &meta_format, &meta_usage, &meta_stride, &meta_generation, &meta_igbp_id, &meta_igbp_slot);

        switch ((int)meta_format) {
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            [[fallthrough]];
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            if (meta_usage & (C2AndroidMemoryUsage::HW_CODEC_READ | C2AndroidMemoryUsage::HW_CODEC_WRITE)) {
                mFormat = ExynosGraphicBufferMeta::get_internal_format((buffer_handle_t)handle);
            } else {
                mFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            }
            break;
        default:
            mFormat = meta_format;
            break;
        }

        uint64_t usage = ExynosGraphicBufferMeta::get_usage((buffer_handle_t)handle);

        int32_t fd = ExynosGraphicBufferMeta::get_video_metadata_fd((buffer_handle_t)handle);
        if (fd >= 0) {
            if (usage & ExynosGraphicBufferUsage::ROIINFO) {
                setMetadata(fd, (sizeof(ExynosVideoMeta) + sizeof(ExynosVideoROIData)));

                if (mMetaData != nullptr) {
                    mMetaData->eType = (ExynosVideoInfoType)(mMetaData->eType | VIDEO_INFO_TYPE_ROI_INFO);
                    mMetaData->data.enc.pRoiData = (unsigned long long)(((char *)mMetaData) + sizeof(ExynosVideoMeta));
                }
            } else {
                setMetadata(fd, sizeof(ExynosVideoMeta), clear);

                if (mMetaData != nullptr) {
                    mMetaData->data.enc.pRoiData = 0;
                }
            }
        }

        if (usage & ExynosGraphicBufferUsage::GDC_MODE) {
            ExynosVideoMeta *meta = (ExynosVideoMeta *)mMetaData;
            if ((meta != nullptr) &&
                (meta->eType & VIDEO_INFO_TYPE_GDC_OTF) &&
                (meta->data.enc.nIsGdcOTF == 1)) {
                meta_stride = meta_width = ExynosGraphicBufferMeta::get_stride((buffer_handle_t)handle);
                meta_height = ExynosGraphicBufferMeta::get_vstride((buffer_handle_t)handle);
            }
        }

        if (usage & GRALLOC_USAGE_HW_TEXTURE) {
            setFlags(GPU_TEXTURE);
        }

        StaticExynosLog(Level::Essential, "ExynosBufferImpl", "[%s] gralloc meta : width(%d), height(%d), stride(%d), "
                                    "format(meta:0x%x / final:0x%x), usage(0x%x / 0x%llx), meta(%p)",
                        __FUNCTION__, meta_width, meta_height, meta_stride, meta_format, mFormat, meta_usage, usage, mMetaData);

        memset((char *)&(mImageInfo), 0, sizeof(mImageInfo));
        mImageInfo.nWidth  = meta_width;
        mImageInfo.nHeight = meta_height;
        mImageInfo.nStride = meta_stride;
        mImageInfo.nFormat = mFormat;
        mImageInfo.stCropInfo.nWidth  = mWidth;
        mImageInfo.stCropInfo.nHeight = mHeight;

        mWidth = meta_stride;
        mUsage = C2AndroidMemoryUsage::FromGrallocUsage(usage).expected;
    }

    Type mType;
    C2Block mBlock;
    std::shared_ptr<C2Buffer> mBuffer;

    /* disable copy constructors */
    ExynosBufferImpl() = delete;
};

ExynosBufferAllocator::ExynosBufferAllocator(
    std::shared_ptr<C2BlockPool>    blockPool,
    C2PlatformAllocatorStore::id_t  allocStoreID,
    C2MemoryUsage                   usage) :  mUsage(usage) {
    mObjName = "ExynosBufferAllocator";
    mbLogOff = false;

    mAllocStoreID = C2AllocatorStore::BAD_ID;

    if (blockPool.get() != nullptr) {
        mBlockPool      = blockPool;
        mAllocStoreID   = allocStoreID;
    }

    mBufferCount = std::make_shared<BufferCount>();
}

BufferAllocRetType ExynosBufferAllocator::alloc(AllocArg &argument) {
    ExynosLogFunctionTrace();

    auto shBlockPool = GET_SHARED_PTR(mBlockPool);
    if (!CHECK_SHARED_PTR(shBlockPool)) {
        return std::make_pair(EXYNOS_ERROR_BAD_STATE, nullptr);
    }

    if (argument.checkLimit != nullptr) {
        int32_t allocCount = (mBufferCount.get() != nullptr)? mBufferCount->get():0;
        argument.limit = argument.checkLimit(allocCount, argument.limit);
    }

    if ((argument.limit > 0) &&
        (getAllocCount() >= argument.limit)) {
        std::this_thread::sleep_for(WAIT_ALLOC_TIME);
        ExynosLogT("[%s] allocation is failed due to over limit", __FUNCTION__);
        return std::make_pair(EXYNOS_ERROR_TRY_AGAIN, nullptr);
    }

    void *handle = nullptr;
    auto delfunc = [bufferCount = mBufferCount](ExynosBuffer *p) {
                        if (p != nullptr) {
                            if (bufferCount.get() != nullptr) {
                                StaticExynosLog(Level::Trace, "ExynosBufferAllocator",
                                                "[free : %p] buffer count: %d", p, bufferCount->dec());
                            }

                            delete (static_cast<ExynosBufferImpl*>(p));
                        }
                   };
    std::shared_ptr<ExynosBuffer> buffer = nullptr;

    switch (mAllocStoreID) {
    case C2PlatformAllocatorStore::ION:
        [[fallthrough]];
    case C2PlatformAllocatorStore::BLOB:
        [[fallthrough]];
    case C2AllocatorStore::DEFAULT_LINEAR:
    {
        std::shared_ptr<C2LinearBlock> c2block = nullptr;
        auto attribute = std::get<LinearBufferAttribute>(argument.attr);

        ExynosLogV("[%s] alloc linear buffer : size(%d), usage(%x)", __FUNCTION__, attribute.mSize, mUsage);

        auto capacity = attribute.mSize + HW_EXTRA_BYTES;

        c2_status_t err = shBlockPool->fetchLinearBlock(capacity, mUsage, &c2block);
        if (err != C2_OK) {
            /* TODO : error handling */
            ExynosLogE("[%s] fetchLinearBlock(%d, 0x%llx) is failed() : 0x%x", __FUNCTION__, capacity, mUsage.expected, err);
            return std::make_pair(EXYNOS_ERROR_TRY_AGAIN, nullptr);
        }

        if (c2block.get() == nullptr) {
            /* TODO : error handling */
            return std::make_pair(EXYNOS_ERROR_UNKNOWN, nullptr);
        }

        handle = new ExynosBufferImpl(c2block, capacity);
    }
        break;
    case C2PlatformAllocatorStore::GRALLOC:
        [[fallthrough]];
    case C2PlatformAllocatorStore::BUFFERQUEUE:
        [[fallthrough]];
    case C2AllocatorStore::DEFAULT_GRAPHIC:
    {
        std::shared_ptr<C2GraphicBlock> c2block = nullptr;
        auto attribute = std::get<GraphicBufferAttribute>(argument.attr);

        if (mUsage.expected & VendorC2Config::MAY_CPU_READ) {
            mUsage.expected = mUsage.expected & (~VendorC2Config::MAY_CPU_READ);

            /* if output is not compressed format, CPU could read output */
            if (!ExynosUtils::CheckCompressedFormat(attribute.mFormat)) {
                mUsage.expected = mUsage.expected | C2MemoryUsage::CPU_READ;
            }
        }

        mUsage.expected = mUsage.expected | attribute.mUsage;

        ExynosLogV("[%s] alloc graphic buffer : width(%d), height(%d), format(0x%x), usage(0x%llx)",
                        __FUNCTION__, attribute.mWidth, attribute.mHeight, attribute.mFormat, mUsage.expected);

        c2_status_t err = shBlockPool->fetchGraphicBlock(attribute.mWidth, attribute.mHeight, attribute.mFormat, mUsage, &c2block);
        if (err != C2_OK) {
            if (mAllocStoreID == C2PlatformAllocatorStore::BUFFERQUEUE) {
                ExynosLogV("[%s] fetchGraphicBlock(%d, %d, 0x%x, 0x%llx) is failed() : 0x%x", __FUNCTION__,
                            attribute.mWidth, attribute.mHeight, attribute.mFormat, mUsage.expected, err);
            } else {
                ExynosLogE("[%s] fetchGraphicBlock(%d, %d, 0x%x, 0x%llx) is failed() : 0x%x", __FUNCTION__,
                            attribute.mWidth, attribute.mHeight, attribute.mFormat, mUsage.expected, err);
            }

            return std::make_pair((err == C2_BLOCKING)? EXYNOS_ERROR_TRY_AGAIN:EXYNOS_ERROR_BAD_STATE, nullptr);
        }

        if (c2block.get() == nullptr) {
            /* TODO : error handling */
            return std::make_pair(EXYNOS_ERROR_UNKNOWN, nullptr);
        }

        handle = new ExynosBufferImpl(c2block, attribute.mWidth, attribute.mHeight, attribute.mFormat);
    }
        break;
    default:
        /* TODO : error handling */
        ExynosLogE("[%s] not supported allocStoreID(0x%x)", __FUNCTION__, mAllocStoreID);
        break;
    }

    if (handle == nullptr) {
        return std::make_pair(EXYNOS_ERROR_INVALID_PARAM, nullptr);
    }

    buffer = std::shared_ptr<ExynosBuffer>(static_cast<ExynosBuffer*>(handle), std::move(delfunc));

    if ((buffer.get() != nullptr) &&
        (mBufferCount.get() != nullptr)) {
        ExynosLogT("[alloc : %p] buffer count: %d", buffer.get(), mBufferCount->inc());
        argument.allocCount = mBufferCount->get();
    }

    return std::make_pair(EXYNOS_ERROR_NONE, buffer);
}

void ExynosBufferAllocator::free(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (!CHECK_SHARED_PTR(buffer)) {
        return;
    }

    auto handle = static_cast<ExynosBufferImpl*>(buffer.get());
    if (handle != nullptr) {
        handle->reset();
    }

    return;
}

std::optional<std::shared_ptr<ExynosBuffer>> importC2BufferLinear(std::shared_ptr<C2Buffer> c2buffer) {
    const C2BufferData data = c2buffer->data();

    C2ConstLinearBlock c2block = data.linearBlocks().front();

    auto handle = new ExynosBufferImpl(c2buffer, const_cast<ExynosBufferHandle *>(c2block.handle()),
                                       c2block.capacity());

    auto delfunc = [](ExynosBuffer *p) {
                        if (p != nullptr) {
                           delete (static_cast<ExynosBufferImpl*>(p));
                        }
                   };

    auto buffer = std::shared_ptr<ExynosBuffer>(static_cast<ExynosBuffer*>(handle), std::move(delfunc));

    StaticExynosLog(Level::Trace, "ExynosBufferAllocator", "[%s] linear : capacity(%d), size(%d)", __FUNCTION__,
            buffer->size(), buffer->getDataLen());

    return { buffer };
}

std::optional<std::shared_ptr<ExynosBuffer>> importC2BufferGraphic(std::shared_ptr<C2Buffer> c2buffer) {
    const C2BufferData data = c2buffer->data();

    C2ConstGraphicBlock c2block = data.graphicBlocks().front();

    auto handle = new ExynosBufferImpl(c2buffer, const_cast<ExynosBufferHandle *>(c2block.handle()),
                                       c2block.width(), c2block.height());

    auto delfunc = [](ExynosBuffer *p) {
                        if (p != nullptr) {
                            delete (static_cast<ExynosBufferImpl*>(p));
                        }
                   };

    auto buffer = std::shared_ptr<ExynosBuffer>(static_cast<ExynosBuffer*>(handle), std::move(delfunc));
    return { buffer };
}

std::optional<std::shared_ptr<ExynosBuffer>> ExynosBufferAllocator::importC2Buffer(std::shared_ptr<C2Buffer> c2buffer) {
    if (!CHECK_SHARED_PTR(c2buffer)) {
        return std::nullopt;
    }

    const C2BufferData data = c2buffer->data();

    switch (data.type()) {
    case C2BufferData::LINEAR:
    {
        return importC2BufferLinear(c2buffer);
    }
        break;
    case C2BufferData::GRAPHIC:
    {
        return importC2BufferGraphic(c2buffer);
    }
        break;
    default:
        /* TODO : error handling */
        StaticExynosLog(Level::Error, "ExynosBufferAllocator", "[%s] type(0x%x) is niether LINEAR(0x%x) or GRAPHIC(0x%x)",
                        __FUNCTION__, data.type(), C2BufferData::LINEAR, C2BufferData::GRAPHIC);
        break;
    }

    return std::nullopt;
}

std::optional<std::shared_ptr<C2Buffer>> ExynosBufferAllocator::exportC2Buffer(std::shared_ptr<ExynosBuffer> buffer) {
    if (!CHECK_SHARED_PTR(buffer)) {
        return std::nullopt;
    }

    auto handle = static_cast<ExynosBufferImpl*>(buffer.get());

    std::shared_ptr<C2Buffer> c2buffer = handle->getC2Buffer();

    /* TODO : need ? */
    // handle.reset();

    if (c2buffer.get() == nullptr) {
        return std::nullopt;
    }

    return { c2buffer };
}

C2BlockPool::local_id_t ExynosBufferAllocator::getBlockPoolID(C2PlatformAllocatorStore::id_t allocStoreID) {
    C2BlockPool::local_id_t ret = C2BlockPool::BASIC_LINEAR;

    switch (allocStoreID) {
    case C2PlatformAllocatorStore::ION:
        [[fallthrough]];
    case C2PlatformAllocatorStore::BLOB:
        [[fallthrough]];
    case C2AllocatorStore::DEFAULT_LINEAR:
        ret = C2BlockPool::BASIC_LINEAR;
        break;
    case C2PlatformAllocatorStore::GRALLOC:
        [[fallthrough]];
    case C2AllocatorStore::DEFAULT_GRAPHIC:
        ret = C2BlockPool::BASIC_GRAPHIC;
        break;
    default:
        ret = C2BlockPool::PLATFORM_START;
        break;
    }

    return ret;
}

C2PlatformAllocatorStore::id_t ExynosBufferAllocator::getAllocatorID(C2PlatformAllocatorStore::id_t allocStoreID) {
    C2PlatformAllocatorStore::id_t ret = C2AllocatorStore::PLATFORM_START;

    switch (allocStoreID) {
    case C2VendorAllocatorStore::GRALLOC_INTERNAL:
        ret = C2AllocatorStore::DEFAULT_GRAPHIC;
        break;
    default:
        break;
    }

    return ret;
}

bool GetCodec2BlockPool(
    std::shared_ptr<const C2Component>  component,
    C2PlatformAllocatorStore::id_t     &allocStoreID,
    C2BlockPool::local_id_t             poolID,
    std::shared_ptr<C2BlockPool>       *blockPool) {
    if (allocStoreID >= C2AllocatorStore::VENDOR_START) {
        /* create a block pool */
        allocStoreID = ExynosBufferAllocator::getAllocatorID(allocStoreID);

        auto c2err = android::CreateCodec2BlockPool(allocStoreID, component, blockPool);
        if (c2err != C2_OK) {
            /* CreateCodec2BlockPool() is failed */
            return false;
        }
    } else {
        /* get a block pool */
        auto selectedPoolID = (poolID >= C2BlockPool::PLATFORM_START)? poolID:ExynosBufferAllocator::getBlockPoolID(allocStoreID);

        auto c2err = android::GetCodec2BlockPool(selectedPoolID, component, blockPool);
        if (c2err != C2_OK) {
            /* GetCodec2BlockPool() is failed */
            return false;
        }
    }

    return true;
}

std::shared_ptr<ExynosBufferAllocator> ExynosBufferAllocator::makeAllocator(
    std::shared_ptr<const C2Component>  component,
    C2PlatformAllocatorStore::id_t      allocStoreID,
    C2BlockPool::local_id_t             poolID,
    C2MemoryUsage                       usage,
    std::shared_ptr<C2BlockPool>       *blockPool) {
    if ((!CHECK_SHARED_PTR(component)) ||
        (blockPool == nullptr)) {
        /* invalid parameter */
        return nullptr;
    }

    if (false == GetCodec2BlockPool(component, allocStoreID, poolID, blockPool)) {
        return nullptr;
    }

    if (!CHECK_SHARED_PTR(*blockPool)) {
        return nullptr;
    }

    /* create a buffer allocator */
#if 0
    auto allocator = std::make_shared<ExynosBufferAllocator>(*blockPool, allocStoreID, usage);
#else
    auto delfunc = [](ExynosBufferAllocator *p) {
                        if (p != nullptr) {
                           delete p;
                        }
                   };

    auto allocator = std::shared_ptr<ExynosBufferAllocator>(new ExynosBufferAllocator(*blockPool, allocStoreID, usage),
                                                    std::move(delfunc));
#endif

    if (!CHECK_SHARED_PTR(allocator)) {
        blockPool->reset();
        return nullptr;
    }

    return allocator;
}

