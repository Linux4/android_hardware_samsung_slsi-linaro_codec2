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
#ifndef EXYNOS_BUFFER_H
#define EXYNOS_BUFFER_H

#include <thread>
#include <sstream>
#include <sys/mman.h>
// #include <errno.h>  ??
#include <mutex>
#include <string>
#include <variant>
#include <utility>
#include <memory>
#include <cutils/native_handle.h>

#include "VendorVideoAPI.h"

#include "ExynosDef.h"
#include "ExynosParam.h"

#include "ExynosETC.h"

#define LOG_ON
#include "ExynosLog.h"

struct LinearBufferAttribute {
    uint32_t mSize;
};

struct GraphicBufferAttribute {
    uint32_t mWidth; /*(= mStride)*/
    uint32_t mHeight;
    uint32_t mFormat;
    uint64_t mUsage;
};

struct AllocArg {
    std::variant<LinearBufferAttribute, GraphicBufferAttribute> attr;
    int limit;
    int allocCount;
    std::function<int32_t(int32_t, int32_t)> checkLimit;
};

struct BufferAddressInfo {
    void            *plane[BASE_BUFFER_MAX_PLANES];
    unsigned int     size[BASE_BUFFER_MAX_PLANES];
    int              num;
};

typedef native_handle_t ExynosBufferHandle;

class ExynosBuffer {
public:
    class ExynosBufferOrigin;

    enum type_t : uint32_t {
        LINEAR,
        GRAPHIC,
        UNKNOWN,
    };

    enum flag_t : uint32_t {
        NONE        = 0,
        REPLICA     = 0x1 << 1,
        MAPPED      = 0x1 << 2,
        CONFIGONLY  = 0x1 << 3,
        GPU_TEXTURE = 0x1 << 4,
    };

    ExynosBuffer() {
        mDataType = UNKNOWN;
        mSize = 0;
        mDataLen = 0;
        mWidth = 0;
        mHeight = 0;
        mFormat = 0;
        mUsage = 0;
        mFlags = 0;
        mMark = nullptr;
        mNotify = nullptr;
        mMetaData = nullptr;
        mMetaSize = 0;
        memset(&mAddressInfo, 0, sizeof(mAddressInfo));
        memset(&mImageInfo, 0, sizeof(mImageInfo));
        mStIno = std::nullopt;
    }

    virtual ~ExynosBuffer() {
        unmap();

        if (mMetaData != nullptr) {
            munmap(mMetaData, mMetaSize);
            mMetaData = nullptr;
            mMetaSize = 0;
        }

        mMark.reset();
        mNotify.reset();
        mParams.reset();
    }

    ExynosBufferHandle* handle() {
        return mHandle;
    }

    ExynosVideoMeta* metadata() {
        return mMetaData;
    }

    uint32_t type() {
        return mDataType;
    }

    uint32_t size() {
        return mSize;
    }

    uint32_t width() {
        return mWidth;
    }

    uint32_t height() {
        return mHeight;
    }

    uint32_t format() {
        return mFormat;
    }

    uint64_t usage() {
        return mUsage;
    }

    uint32_t plane() {
        int cnt = 0;
        unsigned int size[BASE_BUFFER_MAX_PLANES];

        if (type() == GRAPHIC) {
            ExynosUtils::GetYUVPlaneInfo(mWidth, mHeight, mFormat, cnt, size, mFlags);
        } else {
            cnt = 1;
        }

        return (uint32_t)cnt;
    }

    void setDataLen(uint32_t len) {
        mDataLen = (mSize < len)? mSize:len;  /* it can not be over the size of allocated */
    }

    uint32_t getDataLen() {
        return mDataLen;
    }

    void setFlags(uint32_t flag) {
        mFlags |= flag;
    }

    uint32_t getFlags() {
        return mFlags;
    }

    std::shared_ptr<uint64_t> getMark() {
        return mMark;
    }

    void setMark(uint64_t tag) {
        if (mMark.get() != nullptr) {
            StaticExynosLog(Level::Error, "ExynosBuffer",
                                "[%s] marking to tag(%llu) is failed. it is already marked(%llu)", (*mMark), tag);
            return;
        }

        mMark = std::make_shared<uint64_t>(tag);

        return;
    }

    void setStIno(uint64_t st_ino) {
        mStIno = { st_ino };
    }

    std::optional<uint64_t> getStIno() {
        return mStIno;
    }

    void setDestroyNotify(std::shared_ptr<std::function<bool(uint32_t)>> func) {
        std::lock_guard<std::mutex> lock(mMutex);

        mNotify = func;
    }

    virtual bool destroy(uint32_t val = 0) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mNotify.get() != nullptr) {
            auto ret = (*mNotify)(val);
            mNotify.reset();

            return ret;
        }

        return true;
    }

    bool map(BufferAddressInfo &info) {
        std::lock_guard<std::mutex> lock(mMutex);

        memset(&info, 0, sizeof(info));

        if (mHandle == nullptr) {
            /* handle is invalide */
            return false;
        }

        if (mFlags & MAPPED) {  /* already mapped */
            memcpy(&info, &mAddressInfo, sizeof(info));
            return true;
        }

        mFlags |= MAPPED;  /* mark */

        if (mDataType == LINEAR) {
            auto addr = (void *)mmap(nullptr, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mHandle->data[0], 0);
            if (addr == nullptr) {
                /* mmap() is failed */
                return false;
            }

            mAddressInfo.num        = 1;
            mAddressInfo.plane[0]   = addr;
            mAddressInfo.size[0]    = mSize;
        } else {
            int cnt = 0;
            unsigned int size[BASE_BUFFER_MAX_PLANES] = { 0, };

            ExynosUtils::GetYUVPlaneInfo(mWidth, mHeight, mFormat, cnt, size, mFlags);

            mAddressInfo.num = cnt;

            for (int i = 0; i < cnt; i++) {
                auto addr = (void *)mmap(nullptr, size[i], PROT_READ | PROT_WRITE, MAP_SHARED, mHandle->data[i], 0);
                if (addr == nullptr) {
                    /* mmap() is failed */
                    return false;
                }

                mAddressInfo.plane[i]   = addr;
                mAddressInfo.size[i]    = size[i];
            }
        }

        memcpy(&info, &mAddressInfo, sizeof(info));

        return true;
    }

    void unmap() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mFlags & MAPPED) {
            for (int i = 0; i < mAddressInfo.num; i++) {
                if (mAddressInfo.plane[i] != nullptr) {
                    munmap(mAddressInfo.plane[i], mAddressInfo.size[i]);
                }
            }

            memset(&mAddressInfo, 0, sizeof(mAddressInfo));
            mFlags &= ~(MAPPED);  /* clear */
        }
    }

    static void dump(std::shared_ptr<ExynosBuffer> buffer, ExynosDebugType type, std::string name = "unknown") {
        if ((type == EXYNOS_DEBUG_NONE) ||
            (buffer.get() == nullptr) ||
            (buffer->handle() == nullptr)) {
            /* invalid parameter */
            return;
        }

        std::lock_guard<std::mutex> lock(buffer->mMutex);

        {
            std::ostringstream tid;
            tid << std::this_thread::get_id();
            name = name + "_" + tid.str();
        }

        char fileName[256] = { 0, };
        FILE *hFile = nullptr;

        if ((type & EXYNOS_DEBUG_FILE_MASK) == EXYNOS_DEBUG_FILE_WHOLE) {
            sprintf(fileName, "/data/vendor/media/dump_%s_%d_%d.data", name.c_str(), buffer->width(), buffer->height());
            hFile = fopen(fileName, "ab+");
        } else {
            static int cnt = 0;
            sprintf(fileName, "/data/vendor/media/dump_%s_%d_%d_%d.data", name.c_str(), cnt++, buffer->width(), buffer->height());
            hFile = fopen(fileName, "wb+");
        }

        if (hFile == nullptr) {
            StaticExynosLog(Level::Error, "ExynosBuffer", "[%s] fopen(%s) is failed", __FUNCTION__, fileName);
            return;
        }

        if (buffer->type() == LINEAR) {
            int size = buffer->getDataLen();
            char *ptr = (char *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->handle()->data[0], 0);
            if (ptr != nullptr) {
                fwrite(ptr, sizeof(char), size, hFile);

                munmap(ptr, size);
            }
        } else {
            int cnt = 0;
            unsigned int size[BASE_BUFFER_MAX_PLANES] = { 0, };

            ExynosUtils::GetYUVImageInfo(buffer->width(), buffer->height(), buffer->format(), cnt, size, buffer->getFlags());

            for (int i = 0; i < cnt; i++) {
                char *addr = (char *)mmap(nullptr, size[i], PROT_READ | PROT_WRITE, MAP_SHARED, buffer->handle()->data[i], 0);
                if (addr != nullptr) {
                    fwrite(addr, sizeof(char), size[i], hFile);
                    munmap(addr, size[i]);
                }
            }
        }

        fclose(hFile);
    }

    virtual std::shared_ptr<ExynosBufferOrigin> origin() {
        return nullptr;
    }

    /* extra information : attributes information of data owned by buffer */
    ImageInfo mImageInfo;

    /* dynamic configurations with buffer */
    std::shared_ptr<ExynosParams> mParams;

protected:
    void setMetadata(int fd, int32_t size, bool clear = false) {
        if (fd >= 0) {
            mMetaData = (ExynosVideoMeta *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (mMetaData == nullptr) {
                StaticExynosLog(Level::Error, "ExynosBuffer", "[%s] map(ExynosvideoMeta) is failed", __FUNCTION__);
            } else {
                mMetaSize = size;

                if (clear) {
                    memset(mMetaData, 0, size);
                }
            }
        }
    }

    /* inborn information */
    ExynosBufferHandle *mHandle = nullptr;

    type_t   mDataType;
    uint32_t mSize;     /* capacity */
    uint32_t mDataLen;  /* filled */
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
    uint64_t mUsage;
    uint32_t mFlags;

    std::shared_ptr<uint64_t> mMark;
    std::shared_ptr<std::function<bool(uint32_t)>> mNotify;

    ExynosVideoMeta *mMetaData;
    uint32_t         mMetaSize;

    std::mutex          mMutex;
    BufferAddressInfo   mAddressInfo;
    std::optional<uint64_t> mStIno;
};

template<typename T>
struct BufferInfo {
    DataInfo eDataInfo;

    ImageInfo stImageInfo;

    ExynosParams params;

    /* information of buffer */
    void            *pBuffer[BASE_BUFFER_MAX_PLANES];  /* TODO : need ? */
    unsigned long    nFD[BASE_BUFFER_MAX_PLANES];
    unsigned int     nAllocLen[BASE_BUFFER_MAX_PLANES];
    unsigned int     nDataSize[BASE_BUFFER_MAX_PLANES];
    int              nPlane;
    T                obj;

    /* identity on BufferInfo */
    int nID;

    static void reset(struct BufferInfo<T> &obj) {
        obj.eDataInfo = DataInfo::NoData;
        memset(&(obj.stImageInfo), 0, sizeof(obj.stImageInfo));
        obj.params.clear();
        memset(&obj.pBuffer, 0, sizeof(obj.pBuffer));
        memset(&obj.nFD, 0, sizeof(obj.nFD));
        memset(&obj.nAllocLen, 0, sizeof(obj.nAllocLen));
        memset(&obj.nDataSize, 0, sizeof(obj.nDataSize));
        obj.nPlane = 0;
        obj.nID = -1;
    }
};

typedef struct BufferInfo<std::shared_ptr<ExynosBuffer>> ExynosBufferInfo;

typedef std::pair<ExynosErrorType, std::shared_ptr<ExynosBuffer>> BufferAllocRetType;
typedef std::function<BufferAllocRetType(AllocArg &)> BufferAllocFnType;

#endif // EXYNOS_BUFFER_H
