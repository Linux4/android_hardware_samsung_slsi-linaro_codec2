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
#include <C2AllocatorIon.h>
#include <C2AllocatorGralloc.h>
#include <C2BufferPriv.h>
#include <C2PlatformSupport.h>

#include <C2Component.h>
#include <C2ComponentFactory.h>
#include <C2Config.h>
#include <util/C2InterfaceHelper.h>

#include <stdlib.h>
#include <dlfcn.h>

#include <map>
#include <memory>
#include <mutex>

#include "exynos_format.h"
#include "C2ExynosSupport.h"
#include "ExynosDef.h"
#include "ExynosIONUtils.h"
#include "Exynos_C2_ComponentRM.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosC2ComponentStore"

#ifndef MAX_SECURE_RESOURCE
#define MAX_SECURE_RESOURCE 3
#endif
#define MAX_DEC_RESOURCE 16
#define MAX_ENC_RESOURCE 16

static std::mutex gMutex;
static std::shared_ptr<ExynosC2ComponentRM> gComponentRM;
static std::shared_ptr<ExynosC2ComponentInfo> gComponentInfo;

std::shared_ptr<ExynosC2ComponentRM> ExynosC2ComponentRM::getInstance() {
    std::lock_guard<std::mutex> lock(gMutex);

    if (gComponentRM.get() == nullptr) {
        auto delfunc = [](ExynosC2ComponentRM *p) {
                            if (p != nullptr) {
                                delete p;
                            }
                       };
        gComponentRM = std::shared_ptr<ExynosC2ComponentRM>(new ExynosC2ComponentRM(),
                                                            std::move(delfunc));
    }

    return gComponentRM;
}

ExynosC2ComponentRM::ExynosC2ComponentRM() : ExynosLog() {
    mObjName = "ExynosC2ComponentRM";
    mbLogOff = false;

    for (int i = 0; i < RESOURCE_MAX; i++) {
        mResources[i] = std::make_shared<uint32_t>(0);
    }
}

ExynosC2ComponentRM::ComponentResource ExynosC2ComponentRM::getResource(ResourceType type) {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mMutex);

    ComponentResource resource = nullptr;

    uint32_t max = 0;

    switch (type) {
    case DECODER:
        max = MAX_DEC_RESOURCE;
        break;
    case ENCODER:
        max = MAX_ENC_RESOURCE;
        break;
    case SECURE:
        max = MAX_SECURE_RESOURCE;
        break;
    default:
        break;
    }

    if ((max == 0) ||
        (mResources[type].use_count() > max)) {
        return nullptr;
    }

    resource = mResources[type];

    ExynosLogV("[%s] resource(%s) is added. (cnt:%d, max:%d)", __FUNCTION__,
                    mResourceNames[type], (mResources[type].use_count() - 1), max);

    return resource;
}

const std::shared_ptr<ExynosC2ComponentInfo> ExynosC2ComponentInfo::getInstance() {
    std::lock_guard<std::mutex> lock(gMutex);

    if (gComponentInfo.get() == nullptr) {
        auto delfunc = [](ExynosC2ComponentInfo *p) {
                            if (p != nullptr) {
                                delete p;
                            }
                       };
        gComponentInfo = std::shared_ptr<ExynosC2ComponentInfo>(new ExynosC2ComponentInfo(), std::move(delfunc));
    }

    return gComponentInfo;
}

ExynosC2ComponentInfo::ExynosC2ComponentInfo() : ExynosLog()  {
    mObjName = "ExynosC2ComponentInfo";
    mbLogOff = false;

    android::MediaCodecsXmlParser parser;

    parser.parseXmlFilesInSearchDirs({ "media_codecs_c2.xml" }, { "vendor/etc" });
    if (parser.getParsingStatus() != android::OK) {
        ExynosLogW("[%s] can not find \"media_codecs_c2.xml\"", __FUNCTION__);
        return;
    }

    mCodecMap = std::make_shared<android::MediaCodecsXmlParser::CodecMap>(parser.getCodecMap());
    mRoleMap  = std::make_shared<android::MediaCodecsXmlParser::RoleMap>(parser.getRoleMap());
}

class ExynosC2ComponentStore : public C2ComponentStore, public ExynosLog {
public:
    ExynosC2ComponentStore();
    ~ExynosC2ComponentStore() override = default;

    // The implementation of C2ComponentStore.
    C2String getName() const override;
    c2_status_t createComponent(C2String name,
                                std::shared_ptr<C2Component> *const component) override;
    c2_status_t createInterface(C2String name,
                                std::shared_ptr<C2ComponentInterface> *const interface) override;
    std::vector<std::shared_ptr<const C2Component::Traits>> listComponents() override;

    c2_status_t copyBuffer(std::shared_ptr<C2GraphicBuffer> src,
                           std::shared_ptr<C2GraphicBuffer> dst) override;
    c2_status_t query_sm(const std::vector<C2Param*>& stackParams,
                         const std::vector<C2Param::Index>& heapParamIndices,
                         std::vector<std::unique_ptr<C2Param>>* const heapParams) const override;
    c2_status_t config_sm(const std::vector<C2Param*>& params,
                          std::vector<std::unique_ptr<C2SettingResult>>* const failures) override;
    std::shared_ptr<C2ParamReflector> getParamReflector() const override;
    c2_status_t querySupportedParams_nb(
            std::vector<std::shared_ptr<C2ParamDescriptor>>* const params) const override;
    c2_status_t querySupportedValues_sm(
            std::vector<C2FieldSupportedValuesQuery>& fields) const override;

private:
    class ComponentModule : public C2ComponentFactory, public ExynosLog,
                            public std::enable_shared_from_this<ComponentModule> {
    public:
        virtual c2_status_t createComponent(
                c2_node_id_t id, std::shared_ptr<C2Component>* const component,
                ComponentDeleter deleter = std::default_delete<C2Component>()) override;
        virtual c2_status_t createInterface(
                c2_node_id_t id, std::shared_ptr<C2ComponentInterface>* const interface,
                InterfaceDeleter deleter = std::default_delete<C2ComponentInterface>()) override;

        std::shared_ptr<const C2Component::Traits> getTraits();

        ComponentModule() : mInit(C2_NO_INIT),
                            mLibHandle(nullptr),
                            createFactory(nullptr),
                            destroyFactory(nullptr),
                            mComponentFactory(nullptr) {
            mObjName = "ComponentModule";
        }

        c2_status_t init(std::string libPath, bool isSecure = false);

        virtual ~ComponentModule() override;

    protected:
        std::recursive_mutex mLock;
        std::shared_ptr<C2Component::Traits> mTraits;

        c2_status_t mInit;

        void *mLibHandle;
        C2ComponentFactory::CreateCodec2FactoryFunc createFactory;
        C2ComponentFactory::DestroyCodec2FactoryFunc destroyFactory;
        C2ComponentFactory *mComponentFactory;
    };

    class ComponentLoader : public ExynosLog {
    public:
        ComponentLoader(std::string libPath, bool isSecure = false) : mLibPath(libPath), mIsSecure(isSecure) {
            mObjName = "ComponentLoader";
        }

        // ~ComponentLoader();

        c2_status_t getModule(std::shared_ptr<ComponentModule> *const module);
        c2_status_t getComponent(std::shared_ptr<C2Component> *const component);
        c2_status_t getInterface(std::shared_ptr<C2ComponentInterface> *const interface);

    private:
        std::mutex mMutex;
        std::string mLibPath;
        std::weak_ptr<ComponentModule> mModule;
        bool mIsSecure;
    };

    struct Interface : public C2InterfaceHelper {
        std::shared_ptr<C2StoreIonUsageInfo> mIonUsageInfo;
        std::shared_ptr<C2StoreDmaBufUsageInfo> mDmaBufUsageInfo;
        std::shared_ptr<C2StoreFlexiblePixelFormatDescriptorsInfo> mFlexiblePixelFormatInfo;

        Interface(std::shared_ptr<C2ReflectorHelper> reflector) : C2InterfaceHelper(reflector) {
            setDerivedInstance(this);

            struct Setter {
                static C2R setIonUsage(bool /* mayBlock */, C2P<C2StoreIonUsageInfo> &me) {
                    return ExynosIONUtils::setIonUsage(me);
                }

                static C2R setDmaBufUsage(bool /* mayBlock */, C2P<C2StoreDmaBufUsageInfo> &me) {
                    return ExynosIONUtils::setDmaUsage(me);
                }
            };

            addParameter(
                DefineParam(mIonUsageInfo, "ion-usage")
                .withDefault(new C2StoreIonUsageInfo())
                .withFields({
                    C2F(mIonUsageInfo, usage).flags({ C2MemoryUsage::CPU_READ |
                                                      C2MemoryUsage::CPU_WRITE |
                                                      C2MemoryUsage::READ_PROTECTED |
                                                      C2MemoryUsage::WRITE_PROTECTED
                                                    }),
                    C2F(mIonUsageInfo, capacity).inRange(0, UINT32_MAX, 1024),
                    C2F(mIonUsageInfo, heapMask).any(),
                    C2F(mIonUsageInfo, allocFlags).flags({ (int)ExynosIONUtils::getIonUsageMask() }),
                    C2F(mIonUsageInfo, minAlignment).any(),
                })
                .withSetter(Setter::setIonUsage)
                .build());

            addParameter(
                DefineParam(mDmaBufUsageInfo, "dmabuf-usage")
                .withDefault(C2StoreDmaBufUsageInfo::AllocShared(ExynosIONUtils::MAX_HEAP_NAME, 0, 0))
                .withFields({
                    C2F(mDmaBufUsageInfo, m.usage).flags({ C2MemoryUsage::CPU_READ |
                                                           C2MemoryUsage::CPU_WRITE |
                                                           C2MemoryUsage::READ_PROTECTED |
                                                           C2MemoryUsage::WRITE_PROTECTED
                                                         }),
                    C2F(mDmaBufUsageInfo, m.capacity).inRange(0, UINT32_MAX, 1024),
                    C2F(mDmaBufUsageInfo, m.allocFlags).flags({ (int)ExynosIONUtils::getDmaUsageMask() }),
                    C2F(mDmaBufUsageInfo, m.heapName).any(),
                })
                .withSetter(Setter::setDmaBufUsage)
                .build());

#ifdef USE_FLEXIBLE_P010
            {
                C2FlexiblePixelFormatDescriptorStruct values[2] = {
                    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN, 8, C2Color::YUV_420, C2Color::SEMIPLANAR_PACKED },
                    { HAL_PIXEL_FORMAT_YCBCR_P010, 10, C2Color::YUV_420, C2Color::SEMIPLANAR_PACKED },
                };

                auto flexiblePixelFormatInfo = C2StoreFlexiblePixelFormatDescriptorsInfo::AllocShared(2);
                memcpy(flexiblePixelFormatInfo->m.values, values, sizeof(values));

                mReflector->addStructDescriptors<C2FlexiblePixelFormatDescriptorStruct>();

                addParameter(
                    DefineParam(mFlexiblePixelFormatInfo, "flexible-pixel-format")
                    .withConstValue(flexiblePixelFormatInfo)
                    .build());
            }
#endif
        }
    };

    c2_status_t findComponent(std::string name, ComponentLoader **loader);

    void makeComponentInfo();
    void buildComponentList();

    std::map<std::string, std::string> mComponentInfo;
    std::map<C2String, ComponentLoader> mComponents;
    std::shared_ptr<C2ReflectorHelper> mReflector;
    Interface mInterface;
};

ExynosC2ComponentStore::ExynosC2ComponentStore() : mReflector(std::make_shared<C2ReflectorHelper>()),
                                                   mInterface(mReflector) {
    mObjName = "ExynosC2ComponentStore";
    mbLogOff = false;

    ExynosLogFunctionTrace();

    makeComponentInfo();
    buildComponentList();

    if (mComponents.size() == 0) {
        ExynosLogI("[%s] on running for empty service", __FUNCTION__);
    }
}

void ExynosC2ComponentStore::makeComponentInfo() {
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h264.decoder"),
                        std::forward_as_tuple("libExynosC2H264Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h264.decoder.secure"),
                        std::forward_as_tuple("libExynosC2H264Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h264.encoder"),
                        std::forward_as_tuple("libExynosC2H264Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h264.encoder.secure"),
                        std::forward_as_tuple("libExynosC2H264Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.hevc.decoder"),
                        std::forward_as_tuple("libExynosC2HevcDec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.hevc.decoder.secure"),
                        std::forward_as_tuple("libExynosC2HevcDec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.hevc.encoder"),
                        std::forward_as_tuple("libExynosC2HevcEnc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.hevc.encoder.secure"),
                        std::forward_as_tuple("libExynosC2HevcEnc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.mpeg4.decoder"),
                        std::forward_as_tuple("libExynosC2Mpeg4Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.mpeg4.encoder"),
                        std::forward_as_tuple("libExynosC2Mpeg4Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h263.decoder"),
                        std::forward_as_tuple("libExynosC2H263Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.h263.encoder"),
                        std::forward_as_tuple("libExynosC2H263Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vp8.decoder"),
                        std::forward_as_tuple("libExynosC2Vp8Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vp8.encoder"),
                        std::forward_as_tuple("libExynosC2Vp8Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vp9.decoder"),
                        std::forward_as_tuple("libExynosC2Vp9Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vp9.decoder.secure"),
                        std::forward_as_tuple("libExynosC2Vp9Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vp9.encoder"),
                        std::forward_as_tuple("libExynosC2Vp9Enc.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.mpeg2.decoder"),
                        std::forward_as_tuple("libExynosC2Mpeg2Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.vc1.decoder"),
                        std::forward_as_tuple("libExynosC2Vc1Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.wmv.decoder"),
                        std::forward_as_tuple("libExynosC2WmvDec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.av1.decoder"),
                        std::forward_as_tuple("libExynosC2Av1Dec.so"));
    mComponentInfo.emplace(std::piecewise_construct, std::forward_as_tuple("c2.exynos.av1.decoder.secure"),
                        std::forward_as_tuple("libExynosC2Av1Dec.so"));
}

void ExynosC2ComponentStore::buildComponentList() {
    const auto& codecMap = ExynosC2ComponentInfo::getInstance()->getCodecMap();
    if (codecMap.size() == 0) {
        return;
    }

    ExynosLogI("[%s] ==============================", __FUNCTION__);
    ExynosLogI("[%s] == supported component list ==", __FUNCTION__);

    for (const auto& codec : codecMap) {
        auto name = codec.first;
        if (name.find("c2.exynos.") == std::string::npos) {
            continue;  /* ignore */
        }

        auto libName = mComponentInfo.find(name.c_str())->second;

        if (name.find(".secure") != std::string::npos) {
            mComponents.emplace(std::piecewise_construct, std::forward_as_tuple(name.c_str()),
                                std::forward_as_tuple(libName.c_str(), true));
        } else {
            mComponents.emplace(std::piecewise_construct, std::forward_as_tuple(name.c_str()),
                                std::forward_as_tuple(libName.c_str()));
        }

        ExynosLogI("[%s] %s", __FUNCTION__, name.c_str());
    }

    ExynosLogI("[%s] ==============================", __FUNCTION__);

    return;
}

C2String ExynosC2ComponentStore::getName() const {
    ExynosLogFunctionTrace();

    return "vendor.componentStore.exynos";
}

c2_status_t ExynosC2ComponentStore::createComponent(
    C2String                            name,
    std::shared_ptr<C2Component> *const component) {
    ExynosLogFunctionTrace();

    ComponentLoader *loader = nullptr;

    c2_status_t ret = findComponent(name, &loader);

    if ((ret == C2_OK) &&
        (loader != nullptr)) {
        ret = loader->getComponent(component);

        if (ret == C2_OK) {
            ExynosLogD("[%s] component(%s) is created", __FUNCTION__, name.c_str());
        }
    }

    return ret;
}

c2_status_t ExynosC2ComponentStore::createInterface(
    C2String                                     name,
    std::shared_ptr<C2ComponentInterface> *const interface) {
    ExynosLogFunctionTrace();

    ComponentLoader *loader = nullptr;

    c2_status_t ret = findComponent(name, &loader);

    if ((ret == C2_OK) &&
        (loader != nullptr)) {
        ret = loader->getInterface(interface);

        if (ret == C2_OK) {
            ExynosLogD("[%s] interface(%s) is created", __FUNCTION__, name.c_str());
        }
    }

    return ret;
}

std::vector<std::shared_ptr<const C2Component::Traits>> ExynosC2ComponentStore::listComponents() {
    ExynosLogFunctionTrace();

    std::vector<std::shared_ptr<const C2Component::Traits>> list;

    for (auto &productInfo : mComponents) {
        ComponentLoader &loader = productInfo.second;
        std::shared_ptr<ComponentModule> module = nullptr;

        if (C2_OK == loader.getModule(&module)) {
            auto traits = module->getTraits();
            if (traits) {
                list.push_back(traits);
            }
        }
    }
    return list;
}

c2_status_t ExynosC2ComponentStore::copyBuffer(
    std::shared_ptr<C2GraphicBuffer> src,
    std::shared_ptr<C2GraphicBuffer> dst) {
    UNUSED(src);
    UNUSED(dst);

    return C2_OMITTED;
}

c2_status_t ExynosC2ComponentStore::query_sm(
    const std::vector<C2Param*>                 &stackParams,
    const std::vector<C2Param::Index>           &heapParamIndices,
    std::vector<std::unique_ptr<C2Param>> *const heapParams) const {
    return mInterface.query(stackParams, heapParamIndices, C2_MAY_BLOCK, heapParams);
}

c2_status_t ExynosC2ComponentStore::config_sm(
    const std::vector<C2Param*> &params,
    std::vector<std::unique_ptr<C2SettingResult>> *const failures) {
    return mInterface.config(params, C2_MAY_BLOCK, failures);
}

std::shared_ptr<C2ParamReflector> ExynosC2ComponentStore::getParamReflector() const {
    ExynosLogFunctionTrace();

    return mReflector;
}

c2_status_t ExynosC2ComponentStore::querySupportedParams_nb(
    std::vector<std::shared_ptr<C2ParamDescriptor>> *const params) const {
    return mInterface.querySupportedParams(params);
}

c2_status_t ExynosC2ComponentStore::querySupportedValues_sm(
        std::vector<C2FieldSupportedValuesQuery> &fields) const {
    return mInterface.querySupportedValues(fields, C2_MAY_BLOCK);
}

c2_status_t ExynosC2ComponentStore::findComponent(
    std::string       name,
    ComponentLoader **loader) {
    ExynosLogFunctionTrace();

    if (loader == nullptr) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return C2_BAD_VALUE;
    }

    *loader = nullptr;

    auto productInfo = mComponents.find(name);

    if (productInfo == mComponents.end()) {
        return C2_NOT_FOUND;
    }

    *loader = &productInfo->second;

    return C2_OK;
}

c2_status_t ExynosC2ComponentStore::ComponentModule::init(std::string libPath, bool isSecure) {
    ExynosLogFunctionTrace();

    mLibHandle = dlopen(libPath.c_str(), RTLD_NOW|RTLD_NODELETE);
    if (mLibHandle == nullptr) {
        ExynosLogE("[%s] dlopen(%s) is failed", __FUNCTION__, libPath.c_str());
        return C2_CORRUPTED;
    }

    createFactory = (C2ComponentFactory::CreateCodec2FactoryFunc)dlsym(mLibHandle,
                        ((isSecure == true)? "CreateSecureCodec2Factory":"CreateCodec2Factory"));
    destroyFactory = (C2ComponentFactory::DestroyCodec2FactoryFunc)dlsym(mLibHandle, "DestroyCodec2Factory");

    if ((createFactory == nullptr) ||
        (destroyFactory == nullptr)) {
        ExynosLogE("[%s] dlsym() is failed", __FUNCTION__);
        return C2_NO_MEMORY;
    }

    mComponentFactory = createFactory();

    if (mComponentFactory == nullptr) {
        ExynosLogE("[%s] can not create a factory", __FUNCTION__);
        return C2_NO_MEMORY;
    }

    mInit = C2_OK;

    return C2_OK;
}

ExynosC2ComponentStore::ComponentModule::~ComponentModule() {
    if ((destroyFactory != nullptr) &&
        (mComponentFactory != nullptr)) {
        destroyFactory(mComponentFactory);
        mComponentFactory = nullptr;
    }

    if (mLibHandle != nullptr) {
        dlclose(mLibHandle);
        mLibHandle = nullptr;
    }
}

c2_status_t ExynosC2ComponentStore::ComponentModule::createComponent(
    c2_node_id_t                         id,
    std::shared_ptr<C2Component>* const  component,
    std::function<void(::C2Component*)>  deleter) {
    ExynosLogFunctionTrace();

    component->reset();

    if (mInit != C2_OK) {
        ExynosLogE("[%s] undefined error", __FUNCTION__);
        return mInit;
    }

    std::shared_ptr<ComponentModule> module = shared_from_this();

    return mComponentFactory->createComponent(id, component,
                                              [module, deleter](C2Component *p) mutable {
                                                // capture module so that we ensure we still have it while deleting component
                                                deleter(p); // delete component first
                                                module.reset(); // remove module ref (not technically needed)
                                                });
}

c2_status_t ExynosC2ComponentStore::ComponentModule::createInterface(
    c2_node_id_t                                 id,
    std::shared_ptr<C2ComponentInterface>* const interface,
    std::function<void(::C2ComponentInterface*)> deleter) {
    ExynosLogFunctionTrace();

    interface->reset();

    if (mInit != C2_OK) {
        ExynosLogE("[%s] undefined error", __FUNCTION__);
        return mInit;
    }

    std::shared_ptr<ComponentModule> module = shared_from_this();

    return mComponentFactory->createInterface(id, interface,
                                              [module, deleter](C2ComponentInterface *p) mutable {
                                                // capture module so that we ensure we still have it while deleting component
                                                deleter(p); // delete component first
                                                module.reset(); // remove module ref (not technically needed)
                                                });
}

std::shared_ptr<const C2Component::Traits> ExynosC2ComponentStore::ComponentModule::getTraits() {
    ExynosLogFunctionTrace();

    std::unique_lock<std::recursive_mutex> lock(mLock);

    if (!mTraits) {
        std::shared_ptr<C2ComponentInterface> interface = nullptr;

        c2_status_t err = createInterface(0, &interface);
        if (err != C2_OK) {
            ExynosLogE("[%s] createInterface() is failed", __FUNCTION__);
            return nullptr;
        }

        std::shared_ptr<C2Component::Traits> traits(new (std::nothrow) C2Component::Traits);
        if (traits) {
            traits->name = interface->getName();

            C2ComponentKindSetting      kind;
            C2ComponentDomainSetting    domain;

            err = interface->query_vb({ &kind, &domain }, {}, C2_MAY_BLOCK, nullptr);
            bool fixDomain = (err != C2_OK);

            if (err == C2_OK) {
                traits->kind    = kind.value;
                traits->domain  = domain.value;
            } else {
                ExynosLogE("failed to query interface for kind and domain: %d", err);

                traits->kind =
                    (traits->name.find("encoder") != std::string::npos) ? C2Component::KIND_ENCODER :
                    (traits->name.find("decoder") != std::string::npos) ? C2Component::KIND_DECODER :
                    C2Component::KIND_OTHER;
            }

            bool encoder = (traits->kind == C2Component::KIND_ENCODER);
            uint32_t mediaTypeIndex = (encoder == true) ? C2PortMediaTypeSetting::output::PARAM_TYPE : C2PortMediaTypeSetting::input::PARAM_TYPE;
            std::vector<std::unique_ptr<C2Param>> params;

            c2_status_t err = interface->query_vb({}, { mediaTypeIndex }, C2_MAY_BLOCK, &params);
            if (err != C2_OK) {
                ExynosLogE("[%s] query_vb() is failed", __FUNCTION__);
                return nullptr;
            }

            if (params.size() != 1u) {
                ExynosLogE("[%s] size of params is invalid", __FUNCTION__);
                return nullptr;
            }

            C2PortMediaTypeSetting *mediaTypeConfig = C2PortMediaTypeSetting::From(params[0].get());
            if (mediaTypeConfig == nullptr) {
                ExynosLogE("[%s] media type is invalid", __FUNCTION__);
                return nullptr;
            }

            traits->mediaType = std::string(mediaTypeConfig->m.value,
                        strnlen(mediaTypeConfig->m.value, mediaTypeConfig->flexCount()));

            if (fixDomain) {
                if (strncmp(traits->mediaType.c_str(), "video/", 6) == 0) {
                    traits->domain = C2Component::DOMAIN_VIDEO;
                } else if (strncmp(traits->mediaType.c_str(), "image/", 6) == 0) {
                    traits->domain = C2Component::DOMAIN_IMAGE;
                } else {
                    traits->domain = C2Component::DOMAIN_OTHER;
                }
            }

            traits->rank = 128;
        }

        mTraits = traits;
    }

    return mTraits;
}

c2_status_t ExynosC2ComponentStore::ComponentLoader::getModule(std::shared_ptr<ComponentModule> *const module) {
    c2_status_t ret = C2_OK;
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mMutex);

    std::shared_ptr<ComponentModule> localModule = nullptr;

    if (!mModule.expired()) {
        localModule = mModule.lock();
    }

    if (localModule.get() == nullptr) {
        localModule = std::make_shared<ComponentModule>();

        ret = localModule->init(mLibPath, mIsSecure);
        if (ret == C2_OK) {
            mModule = localModule;
        }
    }

    *module = localModule;

    return ret;
}

c2_status_t ExynosC2ComponentStore::ComponentLoader::getComponent(std::shared_ptr<C2Component> *const component) {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    std::shared_ptr<ComponentModule> module = nullptr;

    ret = getModule(&module);
    if (ret == C2_OK) {
        return module->createComponent(0, component);
    }

    return ret;
}

c2_status_t ExynosC2ComponentStore::ComponentLoader::getInterface(std::shared_ptr<C2ComponentInterface> *const interface) {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    std::shared_ptr<ComponentModule> module = nullptr;

    ret = getModule(&module);
    if (ret == C2_OK) {
        module->createInterface(0, interface);
    }

    return ret;
}


namespace android {

std::shared_ptr<C2ComponentStore> GetCodec2ExynosComponentStore() {
    static std::mutex mutex;

    static std::weak_ptr<C2ComponentStore> C2Store;

    std::lock_guard<std::mutex> lock(mutex);

    std::shared_ptr<C2ComponentStore> store = nullptr;

    if (!C2Store.expired()) {
        store = C2Store.lock();
    }

    SetStaticExynosLogLevel();

    if (store.get() == nullptr) {
        store = std::make_shared<ExynosC2ComponentStore>();
        C2Store = store;
    }

    return store;
}

}  // namespace android
