/*
 *
 * Copyright 2019 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_C2_CONSTANTS_H
#define EXYNOS_C2_CONSTANTS_H

constexpr int32_t VENDOR_BITRATE_MODE_START        = 0x10000;
constexpr int32_t VENDOR_BITRATE_MODE_CBR_VTCALL   = 0x10001;
constexpr int32_t VENDOR_BITRATE_MODE_CBR_WFD      = 0x10002;
constexpr int32_t VENDOR_BITRATE_MODE_VBR_BITSAVE  = 0x10003;

/* constexpr int32_t ProfileVendorStart = 0x70000000;  // C2_PROFILE_LEVEL_VENDOR_START */
constexpr int32_t VC1ProfileSimple      = 0x70000000;  // ProfileVendorStart + 0
constexpr int32_t VC1ProfileMain        = 0x70000001;
constexpr int32_t VC1ProfileAdvanced    = 0x70000002;

constexpr int32_t VC1LevelLow       = 0x70000000;
constexpr int32_t VC1LevelMedium    = 0x70000001;
constexpr int32_t VC1LevelHigh      = 0x70000002;
constexpr int32_t VC1Level0         = 0x70000003;
constexpr int32_t VC1Level1         = 0x70000004;
constexpr int32_t VC1Level2         = 0x70000005;
constexpr int32_t VC1Level3         = 0x70000006;
constexpr int32_t VC1Level4         = 0x70000007;

constexpr int32_t VENDOR_COMPRESSED_COLOR_NONE      = 0;
constexpr int32_t VENDOR_COMPRESSED_COLOR_LOSSLESS  = 1;

constexpr int32_t VENDOR_ENTROPY_MODE_CAVLC = 0;
constexpr int32_t VENDOR_ENTROPY_MODE_CABAC = 1;

constexpr int32_t VENDOR_PMV_DISABLED   = 0;
constexpr int32_t VENDOR_PMV_LOCAL      = 1;
constexpr int32_t VENDOR_PMV_GLOBAL     = 2;

constexpr int64_t VENDOR_MEM_USAGE_CPU_READ = 0x100000000;

/* skype */
constexpr int32_t SkypeProfileNone                  = 0;
constexpr int32_t SkypeProfileBaseline              = 0x1;
constexpr int32_t SkypeProfileMain                  = 0x2;
constexpr int32_t SkypeProfileHigh                  = 0x8;
constexpr int32_t SkypeProfileConstrainedBaseline   = 0x10000;
constexpr int32_t SkypeProfileConstrainedHigh       = 0x80000;

constexpr int32_t SkypeLevelNone    = 0;
constexpr int32_t SkypeLevel1       = 0x1;
constexpr int32_t SkypeLevel1B      = 0x2;
constexpr int32_t SkypeLevel1_1     = 0x4;
constexpr int32_t SkypeLevel1_2     = 0x8;
constexpr int32_t SkypeLevel1_3     = 0x10;
constexpr int32_t SkypeLevel2       = 0x20;
constexpr int32_t SkypeLevel2_1     = 0x40;
constexpr int32_t SkypeLevel2_2     = 0x80;
constexpr int32_t SkypeLevel3       = 0x100;
constexpr int32_t SkypeLevel3_1     = 0x200;
constexpr int32_t SkypeLevel3_2     = 0x400;
constexpr int32_t SkypeLevel4       = 0x800;
constexpr int32_t SkypeLevel4_1     = 0x1000;
constexpr int32_t SkypeLevel4_2     = 0x2000;
constexpr int32_t SkypeLevel5       = 0x4000;
constexpr int32_t SkypeLevel5_1     = 0x8000;
constexpr int32_t SkypeLevel5_2     = 0x10000;
constexpr int32_t SkypeLevel6       = 0x20000;
constexpr int32_t SkypeLevel6_1     = 0x40000;
constexpr int32_t SkypeLevel6_2     = 0x80000;

constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_IGNORE          = 0;
constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_VBR             = 1;
constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_CBR             = 2;
constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_VBR_SKIP_FRAME  = 3;
constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_CBR_SKIP_FRAME  = 4;
constexpr int32_t VENDOR_SKYPE_BITRATE_MODE_NONE            = 0x7FFFFFFF;


/* key */
#define GEN_VENDOR_KEY(name) "vendor." name

/* vendor.sec-dec-output.buffers.usage.value */
#define VENDOR_KEY_NAME_OUTPUT_STREAM_USAGE "sec-dec-output.buffers.usage"
constexpr char VENDOR_KEY_OUTPUT_STREAM_USAGE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_OUTPUT_STREAM_USAGE)".value";

/* vendor.sec-dec-output.decode-order.value */
#define VENDOR_KEY_NAME_DECODE_ORDER "sec-dec-output.decode-order"
constexpr char VENDOR_KEY_DECODE_ORDER[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_DECODE_ORDER)".value";

/* vendor.sec-dec-output.reorder-timestamp.value */
#define VENDOR_KEY_NAME_REORDER_TIMESTAMP "sec-dec-output.reorder-timestamp"
constexpr char VENDOR_KEY_REORDER_TIMESTAMP[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_REORDER_TIMESTAMP)".value";

/* vendor.sec-ext-dec-use-buffer-copy.enable.value */
#define VENDOR_KEY_NAME_BUFFER_COPY "sec-ext-dec-use-buffer-copy.enable"
constexpr char VENDOR_KEY_BUFFER_COPY[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BUFFER_COPY)".value";

/* vendor.sec-dec-output.black-bar.enable.value */
#define VENDOR_KEY_NAME_BLACK_BAR "sec-dec-output.black-bar.enable"
constexpr char VENDOR_KEY_BLACK_BAR[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BLACK_BAR)".value";

/* vendor.sec-dec-output.black-bar-info.left
   vendor.sec-dec-output.black-bar-info.top
   vendor.sec-dec-output.black-bar-info.width
   vendor.sec-dec-output.black-bar-info.height */
#define VENDOR_KEY_NAME_BLACK_BAR_INFO "sec-dec-output.black-bar-info"
constexpr char VENDOR_KEY_BLACK_BAR_INFO_LEFT[]   = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BLACK_BAR_INFO)".left";
constexpr char VENDOR_KEY_BLACK_BAR_INFO_TOP[]    = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BLACK_BAR_INFO)".top";
constexpr char VENDOR_KEY_BLACK_BAR_INFO_WIDTH[]  = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BLACK_BAR_INFO)".width";
constexpr char VENDOR_KEY_BLACK_BAR_INFO_HEIGHT[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_BLACK_BAR_INFO)".height";

/* vendor.sec-dec-output.display-delay.value */
#define VENDOR_KEY_NAME_DISPLAY_DELAY "sec-dec-output.display-delay"
constexpr char VENDOR_KEY_DISPLAY_DELAY[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_DISPLAY_DELAY)".value";

/* vendor.sec-dec-output.thumbnail-mode.value */
#define VENDOR_KEY_NAME_THUMBNAIL_MODE "sec-dec-output.thumbnail-mode"
constexpr char VENDOR_KEY_THUMBNAIL_MODE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_THUMBNAIL_MODE)".value";

/* vendor.sec-ext-enc-qp-range.I-minQP
 * vendor.sec-ext-enc-qp-range.I-maxQP
 * vendor.sec-ext-enc-qp-range.P-minQP
 * vendor.sec-ext-enc-qp-range.P-maxQP
 * vendor.sec-ext-enc-qp-range.B-minQP
 * vendor.sec-ext-enc-qp-range.B-maxQP
 */
#define VENDOR_KEY_NAME_QP_RANGE "sec-ext-enc-qp-range"
constexpr char VENDOR_KEY_QP_RANGE_I_MIN[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".I-minQP";
constexpr char VENDOR_KEY_QP_RANGE_I_MAX[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".I-maxQP";
constexpr char VENDOR_KEY_QP_RANGE_P_MIN[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".P-minQP";
constexpr char VENDOR_KEY_QP_RANGE_P_MAX[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".P-maxQP";
constexpr char VENDOR_KEY_QP_RANGE_B_MIN[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".B-minQP";
constexpr char VENDOR_KEY_QP_RANGE_B_MAX[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_QP_RANGE)".B-maxQP";

/* vendor.sec-ext-enc-drop-control.enable.value */
#define VENDOR_KEY_NAME_DROP_CONTROL "sec-ext-enc-drop-control.enable"
constexpr char VENDOR_KEY_DROP_CONTROL[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_DROP_CONTROL)".value";

/* vendor.sec-ext-enc-disable-dfr.disable.value */
#define VENDOR_KEY_NAME_DYNAMIC_FRAMERATE "sec-ext-enc-disable-dfr.disable"
constexpr char VENDOR_KEY_DYNAMIC_FRAMERATE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_DYNAMIC_FRAMERATE)".value";

/* vendor.sec-ext-enc-ref-pframes.number.value */
#define VENDOR_KEY_NAME_REF_P_FRAMES "sec-ext-enc-ref-pframes.number"
constexpr char VENDOR_KEY_REF_P_FRAMES[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_REF_P_FRAMES)".value";

/* vendor.sec-ext-enc-gpb.enable.value */
#define VENDOR_KEY_NAME_GENERAL_PB "sec-ext-enc-gpb.enable"
constexpr char VENDOR_KEY_GENERAL_PB[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_GENERAL_PB)".value";

/* vendor.sec-ext-dec-compressed-color-format.value */
#define VENDOR_KEY_NAME_COMPRESSED_COLOR "sec-ext-dec-compressed-color-format"
constexpr char VENDOR_KEY_COMPRESSED_COLOR[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_COMPRESSED_COLOR)".value";

/* vendor.sec-dec-output.image-convert.value */
#define VENDOR_KEY_NAME_IMAGE_CONVERT "sec-dec-output.image-convert"
constexpr char VENDOR_KEY_IMAGE_CONVERT[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_IMAGE_CONVERT)".value";

/* vendor.sec-ext-imageformat-filter-enableInplace.value */
#define VENDOR_KEY_NAME_IMAGE_CONVERT_MODE "sec-ext-imageformat-filter-enableInplace"
constexpr char VENDOR_KEY_IMAGE_CONVERT_MODE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_IMAGE_CONVERT_MODE)".value";

/* vendor.sec-dec-output.image-convert-pixel-format.value */
#define VENDOR_KEY_NAME_IMAGE_CONVERT_PIXEL_FORMAT "sec-dec-output.image-convert-pixel-format"
constexpr char VENDOR_KEY_IMAGE_CONVERT_PIXEL_FORMAT[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_IMAGE_CONVERT_PIXEL_FORMAT)".value";

/* vendor.sec-dec-output.image-convert-control-frame.value */
#define VENDOR_KEY_NAME_IMAGE_CONVERT_CONTROL_FRAME "sec-dec-output.image-convert-control-frame"
constexpr char VENDOR_KEY_IMAGE_CONVERT_CONTROL_FRAME[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_IMAGE_CONVERT_CONTROL_FRAME)".value";

/* vendor.sec-enc-output.average-qp.value */
#define VENDOR_KEY_NAME_AVERAGE_QP "sec-enc-output.average-qp"
constexpr char VENDOR_KEY_AVERAGE_QP[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_AVERAGE_QP)".value";

/* vendor.sec-enc-output.slice-size.value */
#define VENDOR_KEY_NAME_SLICE_SIZE "sec-enc-output.slice-size"
constexpr char VENDOR_KEY_SLICE_SIZE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SLICE_SIZE)".value";

/* vendor.sec-enc-output.vt-call.value */
#define VENDOR_KEY_NAME_VT_CALL "sec-enc-output.vt-call"
constexpr char VENDOR_KEY_VT_CALL[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_VT_CALL)".value";

/* vendor.sec-enc-output.i-frame-ratio.value */
#define VENDOR_KEY_NAME_I_FRAME_RATIO "sec-enc-output.i-frame-ratio"
constexpr char VENDOR_KEY_I_FRAME_RATIO[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_I_FRAME_RATIO)".value";

/* vendor.sec-ext-enc-chroma-qp-offset.cb
 * vendor.sec-ext-enc-chroma-qp-offset.cr
 */
#define VENDOR_KEY_NAME_CHROMA_QP_OFFSET "sec-ext-enc-chroma-qp-offset"
constexpr char VENDOR_KEY_CHROMA_QP_OFFSET_CB[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_CHROMA_QP_OFFSET)".cb";
constexpr char VENDOR_KEY_CHROMA_QP_OFFSET_CR[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_CHROMA_QP_OFFSET)".cr";

/* vendor.sec-enc-h264-output.entropy.value */
#define VENDOR_KEY_NAME_ENTROPY_MODE "sec-enc-h264-output.entropy"
constexpr char VENDOR_KEY_ENTROPY_MODE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_ENTROPY_MODE)".value";

/* vendor.sec-enc-output-pmv.mode
 * vendor.sec-enc-output-pmv.horizonL0
 * vendor.sec-enc-output-pmv.horizonL1
 * vendor.sec-enc-output-pmv.verticalL0
 * vendor.sec-enc-output-pmv.verticalL1
 */
#define VENDOR_KEY_NAME_PMV "sec-enc-output-pmv"
constexpr char VENDOR_KEY_PMV_MODE[]        = GEN_VENDOR_KEY(VENDOR_KEY_NAME_PMV)".mode";
constexpr char VENDOR_KEY_PMV_HORIZON_L0[]  = GEN_VENDOR_KEY(VENDOR_KEY_NAME_PMV)".horizonL0";
constexpr char VENDOR_KEY_PMV_HORIZON_L1[]  = GEN_VENDOR_KEY(VENDOR_KEY_NAME_PMV)".horizonL1";
constexpr char VENDOR_KEY_PMV_VERTICAL_L0[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_PMV)".verticalL0";
constexpr char VENDOR_KEY_PMV_VERTICAL_L1[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_PMV)".verticalL1";

/* vendor.sec-enc-output.hdr10-plus-stat-enc.value */
#define VENDOR_KEY_NAME_HDR10PLUS_STAT_ENC "sec-enc-output.hdr10-plus-stat-enc"
constexpr char VENDOR_KEY_HDR10PLUS_STAT_ENC[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_HDR10PLUS_STAT_ENC)".value";

/* vendor.sec-enc-output.llwfd-mode */
#define VENDOR_KEY_NAME_LLWFD_MODE  "sec-enc-output.llwfd-mode"
constexpr char VENDOR_KEY_LLWFD_MODE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_LLWFD_MODE)".value";

/* vendor.sec-enc-output.max-i-frame-size.value */
#define VENDOR_KEY_NAME_MAX_I_FRAME_SIZE "sec-enc-output.max-i-frame-size"
constexpr char VENDOR_KEY_MAX_I_FRAME_SIZE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_MAX_I_FRAME_SIZE)".value";


/////////////////////////////////////////////////////
//// SKYPE
/////////////////////////////////////////////////////

/* vendor.rtc-ext-dec-caps-vt-driver-version.number */
#define VENDOR_KEY_NAME_SKYPE_DEC_DRIVER_VERSION "rtc-ext-dec-caps-vt-driver-version"
constexpr char VENDOR_KEY_SKYPE_DEC_DRIVER_VERSION[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_DEC_DRIVER_VERSION)".number";

/* vendor.rtc-ext-enc-caps-vt-driver-version.number */
#define VENDOR_KEY_NAME_SKYPE_ENC_DRIVER_VERSION "rtc-ext-enc-caps-vt-driver-version"
constexpr char VENDOR_KEY_SKYPE_ENC_DRIVER_VERSION[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_DRIVER_VERSION)".number";

/* vendor.rtc-ext-enc-caps-temporal-layers.max-p-count
 * vendor.rtc-ext-enc-caps-temporal-layers.max-b-count
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_MAX_TEMPORAL_LAYER "rtc-ext-enc-caps-temporal-layers"
constexpr char VENDOR_KEY_SKYPE_ENC_MAX_TEMPORAL_LAYER_P[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_MAX_TEMPORAL_LAYER)".max-p-count";
constexpr char VENDOR_KEY_SKYPE_ENC_MAX_TEMPORAL_LAYER_B[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_MAX_TEMPORAL_LAYER)".max-b-count";

/* vendor.rtc-ext-enc-caps-preprocess.max-downscale-factor
 * vendor.rtc-ext-enc-caps-preprocess.rotation
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_PREPROCESS "rtc-ext-enc-caps-preprocess"
constexpr char VENDOR_KEY_SKYPE_ENC_PREPROCESS_DOWNSCALE_FACTOR[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_PREPROCESS)".max-downscale-factor";
constexpr char VENDOR_KEY_SKYPE_ENC_PREPROCESS_ROTATION[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_PREPROCESS)".rotation";

/* vendor.rtc-ext-enc-caps-ltr.max-count */
#define VENDOR_KEY_NAME_SKYPE_ENC_MAX_LTR "rtc-ext-enc-caps-ltr"
constexpr char VENDOR_KEY_SKYPE_ENC_MAX_LTR[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_MAX_LTR)".max-count";

/* vendor.rtc-ext-dec-low-latency.enable
 * vendor.rtc-ext-enc-low-latency.enable
 */
#define VENDOR_KEY_NAME_SKYPE_DEC_LOW_LATENCY "rtc-ext-dec-low-latency"
constexpr char VENDOR_KEY_SKYPE_DEC_LOW_LATENCY[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_DEC_LOW_LATENCY)".enable";
#define VENDOR_KEY_NAME_SKYPE_ENC_LOW_LATENCY "rtc-ext-enc-low-latency"
constexpr char VENDOR_KEY_SKYPE_ENC_LOW_LATENCY[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_LOW_LATENCY)".enable";

/* vendor.rtc-ext-enc-app-input-control.enable */
#define VENDOR_KEY_NAME_SKYPE_ENC_INPUT_CONTROL "rtc-ext-enc-app-input-control"
constexpr char VENDOR_KEY_SKYPE_ENC_INPUT_CONTROL[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_INPUT_CONTROL)".enable";

/* vendor.rtc-ext-enc-custom-profile-level.profile
 * vendor.rtc-ext-enc-custom-profile-level.level
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_CUSTOM_PROFILE_LEVEL "rtc-ext-enc-custom-profile-level"
constexpr char VENDOR_KEY_SKYPE_ENC_CUSTOM_PROFILE[]    = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_CUSTOM_PROFILE_LEVEL)".profile";
constexpr char VENDOR_KEY_SKYPE_ENC_CUSTOM_LEVEL[]      = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_CUSTOM_PROFILE_LEVEL)".level";

/* vendor.rtc-ext-enc-ltr-count.num-ltr-frames */
#define VENDOR_KEY_NAME_SKYPE_ENC_LTR_COUNT "rtc-ext-enc-ltr-count"
constexpr char VENDOR_KEY_SKYPE_ENC_LTR_COUNT[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_LTR_COUNT)".num-ltr-frames";

/* vendor.rtc-ext-enc-ltr.mark-frame
 * vendor.rtc-ext-enc-ltr.use-frame
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_LTR "rtc-ext-enc-ltr"
constexpr char VENDOR_KEY_SKYPE_ENC_LTR_MARK[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_LTR)".mark-frame";
constexpr char VENDOR_KEY_SKYPE_ENC_LTR_USE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_LTR)".use-frame";

/* vendor.rtc-ext-enc-sar.width
 * vendor.rtc-ext-enc-sar.height
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_SAR "rtc-ext-enc-sar"
constexpr char VENDOR_KEY_SKYPE_ENC_SAR_WIDTH[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_SAR)".width";
constexpr char VENDOR_KEY_SKYPE_ENC_SAR_HEIGHT[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_SAR)".height";

/* vendor.rtc-ext-enc-slice.spacing */
#define VENDOR_KEY_NAME_SKYPE_ENC_SLICE_HEADER_SPACING "rtc-ext-enc-slice"
constexpr char VENDOR_KEY_SKYPE_ENC_SLICE_HEADER_SPACING[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_SLICE_HEADER_SPACING)".spacing";

/* vendor.rtc-ext-enc-frame-qp.value */
#define VENDOR_KEY_NAME_SKYPE_ENC_FRAME_QP "rtc-ext-enc-frame-qp"
constexpr char VENDOR_KEY_SKYPE_ENC_FRAME_QP[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_FRAME_QP)".value";

/* vendor.rtc-ext-enc-base-layer-pid.value */
#define VENDOR_KEY_NAME_SKYPE_ENC_BASE_LAYER_PID "rtc-ext-enc-base-layer-pid"
constexpr char VENDOR_KEY_SKYPE_ENC_BASE_LAYER_PID[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_BASE_LAYER_PID)".value";

/* vendor.rtc-ext-enc-bitrate-mode.value
 * vendor.rtc-ext-enc-bitrate-mode.bitrate
 */
#define VENDOR_KEY_NAME_SKYPE_ENC_BITRATE_MODE "rtc-ext-enc-bitrate-mode"
constexpr char VENDOR_KEY_SKYPE_ENC_BITRATE_MODE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_BITRATE_MODE)".value";
constexpr char VENDOR_KEY_SKYPE_ENC_BITRATE_MODE_BITRATE[] = GEN_VENDOR_KEY(VENDOR_KEY_NAME_SKYPE_ENC_BITRATE_MODE)".bitrate";

#endif // EXYNOS_C2_CONSTANTS_H
