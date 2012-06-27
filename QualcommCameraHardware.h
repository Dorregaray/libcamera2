/*
** Copyright 2008, Google Inc.
** Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H

#include "CameraHardwareInterface.h"
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <utils/threads.h>
#include <stdint.h>
#include <ui/OverlayHtc.h>

extern "C" {
#include <linux/android_pmem.h>
#include "msm_camera.h"
}
// Extra propriatary stuff (mostly from CM)
#define MSM_CAMERA_CONTROL "/dev/msm_camera/control0"

#define TRUE 1
#define FALSE 0

typedef struct {
    unsigned int in1_h;
    unsigned int out1_h;
    unsigned int in1_w;
    unsigned int out1_w;
    unsigned int in2_h;
    unsigned int out2_w;
    unsigned int in2_w;
    unsigned int out2_h;
    uint8_t update_flag; 
} common_crop_t;

typedef uint8_t cam_ctrl_type;

#define CEILING16(x) (x&0xfffffff0)
#define CEILING32(x) (x) /* FIXME */
typedef struct {
	//Size: 88 bytes = 44 short
	unsigned short video_width;
	unsigned short video_height;
	unsigned short picture_width;
	unsigned short picture_height;
	unsigned short display_width;
	unsigned short display_height;
	unsigned short orig_picture_dx;
	unsigned short orig_picture_dy;
	unsigned short ui_thumbnail_width;
	unsigned short ui_thumbnail_height;
	unsigned short thumbnail_width;
	unsigned short thumbnail_height;
	unsigned short raw_picture_height;
	unsigned short raw_picture_width;
	unsigned short filler7;
	unsigned short filler8;
	unsigned short filler9;
	unsigned short filler10;
	unsigned short prev_format; //guesss
	unsigned short filler12;
	unsigned short filler13;
	unsigned short filler14;
	unsigned short main_img_format; //guess
	unsigned short enc_format; //guess
	unsigned short thumb_format; //guess
	unsigned short filler18;
	unsigned short filler19;
	unsigned short filler20;
	unsigned short filler21;
	unsigned short filler22;
	unsigned short filler23;
	unsigned short filler24;
	unsigned short filler25;
	unsigned short filler26;
	unsigned short filler27;
	unsigned short filler28;
	unsigned short filler29;
	unsigned short filler30;
	unsigned short filler31;
	unsigned short filler32;
	unsigned short filler33;
	unsigned short filler34;
	unsigned short filler35;
	unsigned short filler36;
} cam_ctrl_dimension_t;

typedef struct {
	uint32_t timestamp;  /* seconds since 1/6/1980          */
	double   latitude;   /* degrees, WGS ellipsoid */
	double   longitude;  /* degrees                */
	int16_t  altitude;   /* meters                          */
} camera_position_type;

typedef uint8_t jpeg_event_t;

typedef enum {
	CAMERA_WB_MIN_MINUS_1,
	CAMERA_WB_AUTO = 1,  /* This list must match aeecamera.h */
	CAMERA_WB_CUSTOM,
	CAMERA_WB_INCANDESCENT,
	CAMERA_WB_FLUORESCENT,
	CAMERA_WB_DAYLIGHT,
	CAMERA_WB_CLOUDY_DAYLIGHT,
	CAMERA_WB_TWILIGHT,
	CAMERA_WB_SHADE,
	CAMERA_WB_MAX_PLUS_1
} camera_wb_type;

typedef enum {
    CAMERA_ANTIBANDING_OFF,
    CAMERA_ANTIBANDING_60HZ,
    CAMERA_ANTIBANDING_50HZ,
    CAMERA_ANTIBANDING_AUTO,
    CAMERA_MAX_ANTIBANDING,
} camera_antibanding_type;

typedef enum {
    CAMERA_BESTSHOT_OFF,
    CAMERA_BESTSHOT_ACTION,
    CAMERA_BESTSHOT_PORTRAIT,
    CAMERA_BESTSHOT_LANDSCAPE,
    CAMERA_BESTSHOT_NIGHT,
    CAMERA_BESTSHOT_NIGHT_PORTRAIT,
    CAMERA_BESTSHOT_THEATRE,
    CAMERA_BESTSHOT_BEACH,
    CAMERA_BESTSHOT_SNOW,
    CAMERA_BESTSHOT_SUNSET,
    CAMERA_BESTSHOT_ANTISHAKE,
    CAMERA_BESTSHOT_FIREWORKS,
    CAMERA_BESTSHOT_SPORTS,
    CAMERA_BESTSHOT_PARTY,
    CAMERA_BESTSHOT_CANDLELIGHT,
    CAMERA_BESTSHOT_BACKLIGHT,
    CAMERA_BESTSHOT_FLOWERS,
} camera_scene_mode_t;

//From now on ... Total guesses ! no RE available afaik.
typedef enum {
	AF_MODE_NORMAL=0,
	AF_MODE_MACRO=1,
	AF_MODE_AUTO=2,
} isp3a_af_mode_t;

enum {
	CAMERA_AEC_FRAME_AVERAGE,
	CAMERA_AEC_CENTER_WEIGHTED,
	CAMERA_AEC_SPOT_METERING,
};

enum {
	LED_MODE_OFF,
	LED_MODE_ON,
	LED_MODE_AUTO,
	LED_MODE_TORCH
};

typedef enum {
	CAMERA_ISO_AUTO,
	CAMERA_ISO_DEBLUR,
	CAMERA_ISO_100,
	CAMERA_ISO_200,
	CAMERA_ISO_400,
	CAMERA_ISO_800,
	CAMERA_ISO_1600,
} camera_iso_mode_type;

struct fifo_queue {
	int num_of_frames;
	int front;
	struct fifo_node *node;
	pthread_mutex_t mut;
	pthread_cond_t wait;
};

struct fifo_node {
	struct msm_frame *f;
	struct fifo_node *next;
};

void enqueue(struct fifo_queue *queue, struct fifo_node *node) {
	struct fifo_node *cur_node=queue->node;
	int i;
	LOGE("enqueue:%p(%d)\n", node, queue->num_of_frames);
	node->next=NULL;
	if(queue->num_of_frames==0) {
		queue->num_of_frames++;
		queue->front=!!queue->num_of_frames;
		queue->node=node;
		return;
	}
	queue->num_of_frames++;
	queue->front=!!queue->num_of_frames;
	for(i=0;i<(queue->num_of_frames-2);++i) {
		cur_node=cur_node->next;
		assert(!!cur_node);
	}
	cur_node->next=node;
}

struct fifo_node *dequeue(struct fifo_queue *queue) {
	if(queue->num_of_frames==0)
		return NULL;
	struct fifo_node *node=queue->node;
	LOGE("dequeue:%p(%d)\n", node, queue->num_of_frames);
	queue->num_of_frames--;
	queue->front=!!queue->num_of_frames;
	queue->node=queue->node->next;
	return node;
}
#define CAMERA_MIN_CONTRAST 0
#define CAMERA_MAX_CONTRAST 255
#define CAMERA_MIN_SHARPNESS 0
#define CAMERA_MAX_SHARPNESS 255
#define CAMERA_MIN_SATURATION 0
#define CAMERA_MAX_SATURATION 255

#define CAMERA_DEF_SHARPNESS 30
#define CAMERA_DEF_CONTRAST 8
#define CAMERA_DEF_SATURATION 6

/* TAG JB 01/20/2010 : From the disassembly of both drem/sapphire + legend camera libraries */
#define CAMERA_SET_PARM_DIMENSION           1
#define CAMERA_SET_PARM_ZOOM                2
#define CAMERA_SET_PARM_SENSOR_POSITION     3   // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FOCUS_RECT          4   // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_LUMA_ADAPTATION     5   // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_CONTRAST            6
#define CAMERA_SET_PARM_BRIGHTNESS          7
#define CAMERA_SET_PARM_EXPOSURE_COMPENSATION   8   // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_SHARPNESS           9   // (4) from liboemcamera.so disassembly
#define CAMERA_SET_PARM_HUE                 10  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_SATURATION          11
#define CAMERA_SET_PARM_EXPOSURE            12
#define CAMERA_SET_PARM_AUTO_FOCUS          13
#define CAMERA_SET_PARM_WB                  14
#define CAMERA_SET_PARM_EFFECT              15
#define CAMERA_SET_PARM_FPS                 16  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FLASH               17  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_NIGHTSHOT_MODE      18  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_REFLECT             19  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_PREVIEW_MODE        20  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_ANTIBANDING         21
#define CAMERA_SET_PARM_RED_EYE_REDUCTION   22  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FOCUS_STEP          23  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_EXPOSURE_METERING   24  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_AUTO_EXPOSURE_MODE  25  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_ISO                 26
#define CAMERA_SET_PARM_BESTSHOT_MODE       27  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_PREVIEW_FPS         29  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_AF_MODE             30  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_HISTOGRAM           31  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FLASH_STATE         32  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FRAME_TIMESTAMP     33  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_STROBE_FLASH        34  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_FPS_LIST            35  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_HJR                 36
#define CAMERA_SET_PARM_ROLLOFF             37
#define CAMERA_STOP_PREVIEW                 38
#define CAMERA_START_PREVIEW                39
#define CAMERA_START_SNAPSHOT               40
#define CAMERA_START_RAW_SNAPSHOT           41
#define CAMERA_STOP_SNAPSHOT                42
#define CAMERA_EXIT                         43
#define CAMERA_GET_PARM_ZOOM                46  // from liboemcamera.so (307Kb version) disassembly
#define CAMERA_GET_PARM_MAXZOOM             47
#define CAMERA_GET_PARM_AF_SHARPNESS        48  // from liboemcamera.so disassembly
#define CAMERA_SET_PARM_LED_MODE            49
#define CAMERA_SET_MOTION_ISO               50  // from liboemcamera.so disassembly
#define CAMERA_AUTO_FOCUS_CANCEL            51  // (38) from liboemcamera.so disassembly
#define CAMERA_GET_PARM_FOCUS_STEP          52  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_ENABLE_AFD                   53  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_PREPARE_SNAPSHOT             54
#define CAMERA_SET_PARM_COORDINATE          55  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_SET_AWB_CALIBRATION          56  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_SET_PARM_LA_MODE             57  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_SET_PARM_AE_COORDINATE       58  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_GET_PARM_FOCAL_LENGTH        59  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_GET_PARM_HORIZONTAL_VIEW_ANGLE 60  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_GET_PARM_VERTICAL_VIEW_ANGLE 61  // from liboemcamera.so (1535Kb version) disassembly
#define CAMERA_GET_PARM_ISO                 62
#define CAMERA_SET_PARM_FRONT_CAMERA_MODE   63  // from liboemcamera.so (1535Kb version) disassembly

#define CAMERA_START_VIDEO                  56
#define CAMERA_STOP_VIDEO                   57
#define CAMERA_START_RECORDING              58
#define CAMERA_STOP_RECORDING               59
/* End of TAG */

#define CAMERA_GET_PARM_ZOOMRATIOS            0  /* FIXME */
#define CAMERA_SET_FPS_MODE                   55 /* FIXME */
#define CAMERA_SET_PARM_SCENE_MODE            0  /* FIXME */
#define CAMERA_SET_PARM_AEC_ROI               61 /* CHECKME */
#define CAMERA_SET_CAF                        62 /* CHECKME */
#define CAMERA_SET_PARM_BL_DETECTION_ENABLE   63 /* CHECKME */
#define CAMERA_SET_PARM_SNOW_DETECTION_ENABLE 64 /* CHECKME */
#define CAMERA_SET_PARM_AF_ROI                65 /* CHECKME */
#define CAMERA_START_LIVESHOT                 0  /* FIXME */


#define CAM_CTRL_SUCCESS 1
#define CAM_CTRL_INVALID_PARM 2 /* FIXME */

#define PAD_TO_WORD(x) ((x&1) ? x+1 : x)
#define JPEG_EVENT_DONE 0 /* useless */



typedef enum {
	CAMERA_RSP_CB_SUCCESS,    /* Function is accepted         */
	CAMERA_EXIT_CB_DONE,      /* Function is executed         */
	CAMERA_EXIT_CB_FAILED,    /* Execution failed or rejected */
	CAMERA_EXIT_CB_DSP_IDLE,  /* DSP is in idle state         */
	CAMERA_EXIT_CB_DSP_ABORT, /* Abort due to DSP failure     */
	CAMERA_EXIT_CB_ABORT,     /* Function aborted             */
	CAMERA_EXIT_CB_ERROR,     /* Failed due to resource       */
	CAMERA_EVT_CB_FRAME,      /* Preview or video frame ready */
	CAMERA_EVT_CB_PICTURE,    /* Picture frame ready for multi-shot */
	CAMERA_STATUS_CB,         /* Status updated               */
	CAMERA_EXIT_CB_FILE_SIZE_EXCEEDED, /* Specified file size not achieved,
	encoded file written & returned anyway */
	CAMERA_EXIT_CB_BUFFER,    /* A buffer is returned         */
	CAMERA_EVT_CB_SNAPSHOT_DONE,/*  Snapshot updated               */
	CAMERA_CB_MAX
} camera_cb_type;

struct cam_frame_start_parms {
	unsigned int prout;
	struct msm_frame frame;
	struct msm_frame video_frame;
};

#define EXIFTAGID_GPS_LATITUDE_REF        0x10001
#define EXIFTAGID_GPS_LATITUDE            0x20002
#define EXIFTAGID_GPS_LONGITUDE_REF       0x30003
#define EXIFTAGID_GPS_LONGITUDE           0x40004
#define EXIFTAGID_GPS_ALTITUDE_REF        0x50005
#define EXIFTAGID_GPS_ALTITUDE            0x60006
#define EXIFTAGID_GPS_TIMESTAMP           0x70007
#define EXIFTAGID_GPS_DATESTAMP           0x0001D /* FIXME */
#define EXIFTAGID_EXIF_CAMERA_MAKER       0x21010F
#define EXIFTAGID_EXIF_CAMERA_MODEL       0x220110
#define EXIFTAGID_EXIF_DATE_TIME_ORIGINAL 0x3A9003
#define EXIFTAGID_EXIF_DATE_TIME          0x3B9004
typedef unsigned int exif_tag_id_t;

#define EXIF_RATIONAL 5
#define EXIF_ASCII 2
#define EXIF_BYTE 1
typedef unsigned int exif_tag_type_t;

typedef struct {
    int val;
    int otherval;
} rat_t;

typedef union {
    char * _ascii; /* At byte 16 relative to exif_tag_entry_t */
    rat_t * _rats;
    rat_t  _rat;
    uint8_t _byte;
} exif_tag_data_t;

/* The entire exif_tag_entry_t struct must be 24 bytes in length */
typedef struct {
    exif_tag_type_t type;
    uint32_t copy;
    uint32_t count;
    exif_tag_data_t data;
} exif_tag_entry_t;

typedef struct {
    exif_tag_id_t tagid;
    exif_tag_entry_t tag_entry;
} exif_tags_info_t;

enum {
    CAMERA_YUV_420_NV12,
    CAMERA_YUV_420_NV21,
    CAMERA_YUV_420_NV21_ADRENO
};

typedef enum {
    AUTO,
    SPOT,
    CENTER_WEIGHTED,
    AVERAGE
} select_zone_af_t;

// End of closed stuff

struct str_map {
    const char *const desc;
    int val;
};

typedef enum {
    TARGET_MSM7625,
    TARGET_MSM7627,
    TARGET_QSD8250,
    TARGET_MSM7630,
    TARGET_MSM8660,
    TARGET_MAX
}targetType;

typedef enum {
    LIVESHOT_DONE,
    LIVESHOT_IN_PROGRESS,
    LIVESHOT_STOPPED
}liveshotState;

struct target_map {
    const char *targetStr;
    targetType targetEnum;
};

struct board_property{
    targetType target;
    unsigned int previewSizeMask;
    bool hasSceneDetect;
    bool hasSelectableZoneAf;
};

namespace android {

class QualcommCameraHardware : public CameraHardwareInterface {
public:

    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual void setCallbacks(notify_callback notify_cb,
                              data_callback data_cb,
                              data_callback_timestamp data_cb_timestamp,
                              void* user);
    virtual void enableMsgType(int32_t msgType);
    virtual void disableMsgType(int32_t msgType);
    virtual bool msgTypeEnabled(int32_t msgType);

    virtual status_t dump(int fd, const Vector<String16>& args) const;
    virtual status_t startPreview();
    virtual void stopPreview();
    virtual bool previewEnabled();
    virtual status_t startRecording();
    virtual void stopRecording();
    virtual bool recordingEnabled();
    virtual void releaseRecordingFrame(const sp<IMemory>& mem);
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t takePicture();
    virtual status_t takeLiveSnapshot();
    void set_liveshot_exifinfo();
    virtual status_t cancelPicture();
    virtual status_t setParameters(const CameraParameters& params);
    virtual CameraParameters getParameters() const;
    virtual status_t sendCommand(int32_t command, int32_t arg1, int32_t arg2);
    virtual status_t getBufferInfo(sp<IMemory>& Frame, size_t *alignedSize);
    virtual void encodeData();

    virtual void release();
    virtual bool useOverlay();
    virtual status_t setOverlay(const sp<Overlay> &overlay);

    static sp<CameraHardwareInterface> createInstance();
    static sp<QualcommCameraHardware> getInstance();

    void receivePreviewFrame(struct msm_frame *frame);
    void receiveLiveSnapshot(uint32_t jpeg_size);
    void receiveRecordingFrame(struct msm_frame *frame);
    void receiveJpegPicture(void);
    void jpeg_set_location();
    void receiveJpegPictureFragment(uint8_t *buf, uint32_t size);
    void notifyShutter(common_crop_t *crop, bool mPlayShutterSoundOnly);
    void receive_camframetimeout();
    static void getCameraInfo();

private:
    QualcommCameraHardware();
    virtual ~QualcommCameraHardware();
    status_t startPreviewInternal();
    void stopPreviewInternal();
    friend void *auto_focus_thread(void *user);
    void runAutoFocus();
    status_t cancelAutoFocusInternal();
    bool native_set_dimension (int camfd);
    bool native_jpeg_encode (void);
    bool native_set_parm(cam_ctrl_type type, uint16_t length, void *value);
    bool native_set_parm(cam_ctrl_type type, uint16_t length, void *value, int *result);
    bool native_zoom_image(int fd, int srcOffset, int dstOffset, common_crop_t *crop);

    static wp<QualcommCameraHardware> singleton;

    /* These constants reflect the number of buffers that libmmcamera requires
       for preview and raw, and need to be updated when libmmcamera
       changes.
    */
    static const int kPreviewBufferCount = NUM_PREVIEW_BUFFERS;
    static const int kRawBufferCount = 1;
    static const int kJpegBufferCount = 1;

    int jpegPadding;

    CameraParameters mParameters;
    unsigned int frame_size;
    bool mCameraRunning;
    Mutex mCameraRunningLock;
    bool mPreviewInitialized;

    // This class represents a heap which maintains several contiguous
    // buffers.  The heap may be backed by pmem (when pmem_pool contains
    // the name of a /dev/pmem* file), or by ashmem (when pmem_pool == NULL).

    struct MemPool : public RefBase {
        MemPool(int buffer_size, int num_buffers,
                int frame_size,
                const char *name);

        virtual ~MemPool() = 0;

        void completeInitialization();
        bool initialized() const {
            return mHeap != NULL && mHeap->base() != MAP_FAILED;
        }

        virtual status_t dump(int fd, const Vector<String16>& args) const;

        int mBufferSize;
        int mAlignedBufferSize;
        int mNumBuffers;
        int mFrameSize;
        sp<MemoryHeapBase> mHeap;
        sp<MemoryBase> *mBuffers;

        const char *mName;
    };

    struct AshmemPool : public MemPool {
        AshmemPool(int buffer_size, int num_buffers,
                   int frame_size,
                   const char *name);
    };

    struct PmemPool : public MemPool {
        PmemPool(const char *pmem_pool,
                 int control_camera_fd, int flags, int pmem_type,
                 int buffer_size, int num_buffers,
                 int frame_size, int cbcr_offset,
                 int yoffset, const char *name);
        virtual ~PmemPool();
        int mFd;
        int mPmemType;
        int mCbCrOffset;
        int myOffset;
        int mCameraControlFd;
        uint32_t mAlignedSize;
        struct pmem_region mSize;
    };

    sp<PmemPool> mPreviewHeap;
    sp<PmemPool> mRecordHeap;
    sp<PmemPool> mThumbnailHeap;
    sp<PmemPool> mRawHeap;
    sp<PmemPool> mDisplayHeap;
    sp<AshmemPool> mJpegHeap;
    sp<PmemPool> mRawSnapShotPmemHeap;
    sp<PmemPool> mPostViewHeap;

    bool startCamera();
    bool initPreview();
    bool initRecord();
    void deinitPreview();
    bool initRaw(bool initJpegHeap);
    bool initLiveSnapshot(int videowidth, int videoheight);
    bool initRawSnapshot();
    void deinitRaw();
    void deinitRawSnapshot();

    bool mFrameThreadRunning;
    Mutex mFrameThreadWaitLock;
    Condition mFrameThreadWait;
    friend void *frame_thread(void *user);
    void runFrameThread(void *data);

    //720p recording video thread
    bool mVideoThreadExit;
    bool mVideoThreadRunning;
    Mutex mVideoThreadWaitLock;
    Condition mVideoThreadWait;
    friend void *video_thread(void *user);
    void runVideoThread(void *data);


    bool mShutterPending;
    Mutex mShutterLock;

    bool mSnapshotThreadRunning;
    Mutex mSnapshotThreadWaitLock;
    Condition mSnapshotThreadWait;
    friend void *snapshot_thread(void *user);
    void runSnapshotThread(void *data);
    Mutex mRawPictureHeapLock;
    bool mJpegThreadRunning;
    Mutex mJpegThreadWaitLock;
    Condition mJpegThreadWait;
    bool mInSnapshotMode;
    Mutex mInSnapshotModeWaitLock;
    Condition mInSnapshotModeWait;
    bool mEncodePending;
    Mutex mEncodePendingWaitLock;
    Condition mEncodePendingWait;

    void debugShowPreviewFPS() const;
    void debugShowVideoFPS() const;

    int mSnapshotFormat;
    bool mFirstFrame;
    void filterPictureSizes();
    void filterPreviewSizes();
    void storeTargetType();
    bool supportsSceneDetection();
    bool supportsSelectableZoneAf();

    void initDefaultParameters();
    void findSensorType();

    status_t setPreviewSize(const CameraParameters& params);
    status_t setJpegThumbnailSize(const CameraParameters& params);
    status_t setPreviewFrameRate(const CameraParameters& params);
    status_t setPreviewFrameRateMode(const CameraParameters& params);
    status_t setRecordSize(const CameraParameters& params);
    status_t setPictureSize(const CameraParameters& params);
    status_t setJpegQuality(const CameraParameters& params);
    status_t setAntibanding(const CameraParameters& params);
    status_t setEffect(const CameraParameters& params);
    status_t setExposureCompensation(const CameraParameters &params);
    status_t setAutoExposure(const CameraParameters& params);
    status_t setWhiteBalance(const CameraParameters& params);
    status_t setFlash(const CameraParameters& params);
    status_t setGpsLocation(const CameraParameters& params);
    status_t setRotation(const CameraParameters& params);
    status_t setZoom(const CameraParameters& params);
    status_t setFocusMode(const CameraParameters& params);
    status_t setBrightness(const CameraParameters& params);
    status_t setSkinToneEnhancement(const CameraParameters& params);
    status_t setOrientation(const CameraParameters& params);
    status_t setLensshadeValue(const CameraParameters& params);
    status_t setISOValue(const CameraParameters& params);
    status_t setPictureFormat(const CameraParameters& params);
    status_t setSharpness(const CameraParameters& params);
    status_t setContrast(const CameraParameters& params);
    status_t setSaturation(const CameraParameters& params);
    status_t setSceneMode(const CameraParameters& params);
    status_t setContinuousAf(const CameraParameters& params);
    status_t setTouchAfAec(const CameraParameters& params);
    status_t setSceneDetect(const CameraParameters& params);
    status_t setStrTextures(const CameraParameters& params);
    status_t setPreviewFormat(const CameraParameters& params);
    status_t setSelectableZoneAf(const CameraParameters& params);
    void setGpsParameters();
    bool storePreviewFrameForPostview();
    bool isValidDimension(int w, int h);

    Mutex mLock;
    Mutex mCamframeTimeoutLock;
    bool camframe_timeout_flag;
    bool mReleasedRecordingFrame;

    bool receiveRawPicture(void);
    bool receiveRawSnapshot(void);

    Mutex mCallbackLock;
    Mutex mOverlayLock;
	Mutex mRecordLock;
	Mutex mRecordFrameLock;
	Condition mRecordWait;
    Condition mStateWait;

    /* mJpegSize keeps track of the size of the accumulated JPEG.  We clear it
       when we are about to take a picture, so at any time it contains either
       zero, or the size of the last JPEG picture taken.
    */
    uint32_t mJpegSize;
    unsigned int        mPreviewFrameSize;
    unsigned int        mRecordFrameSize;
    int                 mRawSize;
    int                 mCbCrOffsetRaw;
    int                 mJpegMaxSize;

#if DLOPEN_LIBMMCAMERA
    void *libmmcamera;
#endif

    int mCameraControlFd;
    struct msm_camsensor_info mSensorInfo;
    cam_ctrl_dimension_t mDimension;
    bool mAutoFocusThreadRunning;
    Mutex mAutoFocusThreadLock;
    int mAutoFocusFd;

    Mutex mAfLock;

    pthread_t mFrameThread;
    pthread_t mVideoThread;
    pthread_t mSnapshotThread;

    common_crop_t mCrop;

    bool mInitialized;

    int mBrightness;
    int mSkinToneEnhancement;
    int mHJR;
    struct msm_frame frames[kPreviewBufferCount];
    struct msm_frame *recordframes;
    bool *record_buffers_tracking_flag;
    bool mInPreviewCallback;
    bool mUseOverlay;
    sp<Overlay>  mOverlay;

    int32_t mMsgEnabled;    // camera msg to be handled
    notify_callback mNotifyCallback;
    data_callback mDataCallback;
    data_callback_timestamp mDataCallbackTimestamp;
    void *mCallbackCookie;  // same for all callbacks
    int mDebugFps;
    int kPreviewBufferCountActual;
    int previewWidth, previewHeight;
    bool mSnapshotDone;
    bool mSnapshotPrepare;
    int videoWidth, videoHeight;

    bool mDisEnabled;
    int mRotation;
    bool mResetOverlayCrop;
    int mThumbnailWidth, mThumbnailHeight;
    status_t setVpeParameters();
    status_t setDIS();
};

}; // namespace android

#endif
