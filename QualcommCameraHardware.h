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
	unsigned int in1_w;
	unsigned int in1_h;
	unsigned int out1_w;
	unsigned int out1_h;
	unsigned int in2_w;
	unsigned int in2_h;
	unsigned int out2_w;
	unsigned int out2_h;
	uint8_t update_flag; 
} common_crop_t;

typedef uint8_t cam_ctrl_type;

#define CEILING16(x) (x&0xfffffff0)
typedef struct {
	//Size: 0x20 bytes = 32 bytes = 16 short
	unsigned short video_width;//0xe4
	unsigned short video_height;//0xe6
	unsigned short picture_width; //0xe8
	unsigned short picture_height;//0xea
	unsigned short display_width; //0xec
	unsigned short display_height; //0xee
	unsigned short orig_picture_dx;  //0xf0
	unsigned short orig_picture_dy; //0xf2
	unsigned short ui_thumbnail_width; //0xf4
	unsigned short ui_thumbnail_height; //0xf6
	unsigned short thumbnail_width;  //0xf8
	unsigned short thumbnail_height;  //0xfa
	unsigned short raw_picture_height; //0xfc
	unsigned short raw_picture_width;  //0xfe
	unsigned short filler7;   ///0x100
	unsigned short filler8;   //0x102
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
	AF_MODE_NORMAL,
	AF_MODE_MACRO,
	AF_MODE_AUTO,
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

enum camera_ops {
    CAMERA_SET_PARM_ENCODE_ROTATION=0,
    CAMERA_SET_PARM_DIMENSION=1,
    CAMERA_SET_PARM_ZOOM=2,
    CAMERA_SET_PARM_SENSOR_POSITION,
    CAMERA_SET_PARM_SHARPNESS=4,
    CAMERA_SET_PARM_LUMA_ADAPTATION,
    CAMERA_SET_PARM_CONTRAST=6,
    CAMERA_SET_PARM_BRIGHTNESS=7,
    CAMERA_SET_PARM_EXPOSURE_COMPENSATION,
    CAMERA_SET_PARM_FOCUS_RECT,
    CAMERA_SET_PARM_HUE=10,
    CAMERA_SET_PARM_SATURATION=11,
    CAMERA_SET_PARM_EXPOSURE=12,
    CAMERA_SET_PARM_AUTO_FOCUS=13,
    CAMERA_SET_PARM_WB=14,
    CAMERA_SET_PARM_EFFECT=15,
    CAMERA_SET_PARM_FPS,
    CAMERA_SET_PARM_FLASH,
    CAMERA_SET_PARM_NIGHTSHOT_MODE,
    CAMERA_SET_PARM_REFLECT,
    CAMERA_SET_PARM_PREVIEW_MODE=20,
    CAMERA_SET_PARM_ANTIBANDING=21,
    CAMERA_SET_PARM_RED_EYE_REDUCTION,
    CAMERA_SET_PARM_FOCUS_STEP,
    CAMERA_SET_PARM_EXPOSURE_METERING,
    CAMERA_SET_PARM_AUTO_EXPOSURE_MODE,
    CAMERA_SET_PARM_ISO=26,
    CAMERA_SET_PARM_BESTSHOT_MODE,
    CAMERA_SET_PARM_PREVIEW_FPS,
    CAMERA_SET_PARM_AF_MODE,
    CAMERA_SET_PARM_HISTOGRAM,
    CAMERA_SET_PARM_FLASH_STATE,
    CAMERA_SET_PARM_FRAME_TIMESTAMP,
    CAMERA_SET_PARM_STROBE_FLASH,
    CAMERA_SET_PARM_FPS_LIST,
    CAMERA_SET_PARM_HJR=36,
    CAMERA_SET_PARM_ROLLOFF=37,
    CAMERA_STOP_PREVIEW=38,
    CAMERA_START_PREVIEW=39,
    CAMERA_START_SNAPSHOT=40,
    CAMERA_START_VIDEO,
    CAMERA_STOP_SNAPSHOT=42,
    CAMERA_EXIT=43,
    CAMERA_STOP_VIDEO,
    CAMERA_START_RECORDING,
    CAMERA_STOP_RECORDING,
    CAMERA_GET_PARM_MAXZOOM=47,
    CAMERA_START_RAW_SNAPSHOT,
    CAMERA_SET_PARM_LED_MODE=49,
    CAMERA_GET_PARM_AF_SHARPNESS,
    CAMERA_SET_MOTION_ISO,
    CAMERA_AUTO_FOCUS_CANCEL,
    CAMERA_GET_PARM_FOCUS_STEP,
    CAMERA_ENABLE_AFD,
    CAMERA_PREPARE_SNAPSHOT,
    CAMERA_SET_FPS_MODE,
    CAMERA_SET_PARM_SCENE_MODE
};

//#define CAMERA_ENABLE_AFD 21 //Bah can't find the real value
//#define CAMERA_AUTO_FOCUS_CANCEL 38
//#define CAMERA_START_RAW_SNAPSHOT 41
//#define CAMERA_PREPARE_SNAPSHOT 54
//#define CAMERA_START_VIDEO 56
//#define CAMERA_STOP_VIDEO 57
//#define CAMERA_START_RECORDING 58
//#define CAMERA_STOP_RECORDING 59

#define CAM_CTRL_SUCCESS 1

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

typedef struct {
	unsigned int a;
	unsigned int b;
} rat_t;

typedef unsigned int exif_tag_id_t;
enum {
	EXIFTAGID_GPS_LATITUDE_REF        = 0x10001,
	EXIFTAGID_GPS_LATITUDE            = 0x20002,
	EXIFTAGID_GPS_LONGITUDE_REF       = 0x30003,
	EXIFTAGID_GPS_LONGITUDE           = 0x40004,
	EXIFTAGID_GPS_ALTITUDE_REF        = 0x50005,
	EXIFTAGID_GPS_ALTITUDE            = 0x60006,
	EXIFTAGID_EXIF_CAMERA_MAKER       = 0x21010F,
	EXIFTAGID_EXIF_CAMERA_MODEL       = 0x220110,
	EXIFTAGID_EXIF_DATE_TIME_ORIGINAL = 0x3A9003,
	EXIFTAGID_EXIF_DATE_TIME          = 0x3B9004,
};
#define EXIF_RATIONAL 5
#define EXIF_ASCII 2
#define EXIF_BYTE 1
typedef unsigned int exif_tag_type_t;
typedef struct {
	//24 bytes = 6 ints
	exif_tag_type_t type;
	uint32_t count;
	uint32_t copy;
	uint32_t junk1;
	uint32_t junk2;
	uint32_t junk3;
} exif_tag_entry_t;

typedef struct {
	exif_tag_id_t tagid;
	exif_tag_entry_t tag_entry;
} exif_tags_info_t;

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

struct target_map {
    const char *targetStr;
    targetType targetEnum;
};

struct board_property{
    targetType target;
    unsigned int previewSizeMask;
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
    virtual status_t cancelPicture();
    virtual status_t setParameters(const CameraParameters& params);
    virtual CameraParameters getParameters() const;
    virtual status_t sendCommand(int32_t command, int32_t arg1, int32_t arg2);
    virtual status_t getBufferInfo( sp<IMemory>& Frame, size_t *alignedSize);

    virtual void release();
    virtual bool useOverlay();
    virtual status_t setOverlay(const sp<Overlay> &overlay);

    static sp<CameraHardwareInterface> createInstance();
    static sp<QualcommCameraHardware> getInstance();

    void receivePreviewFrame(struct msm_frame *frame);
    void receiveRecordingFrame(struct msm_frame *frame);
    void receiveJpegPicture(void);
    void jpeg_set_location();
    void receiveJpegPictureFragment(uint8_t *buf, uint32_t size);
    void notifyShutter(common_crop_t *crop, bool mPlayShutterSoundOnly);
    void receive_camframetimeout();

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
    sp<AshmemPool> mRawSnapshotAshmemHeap;
    sp<PmemPool> mPostViewHeap;


    bool startCamera();
    bool initPreview();
    bool initRecord();
    void deinitPreview();
    bool initRaw(bool initJpegHeap);
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

    void debugShowPreviewFPS() const;
    void debugShowVideoFPS() const;

    int mSnapshotFormat;
    void filterPictureSizes();
    void filterPreviewSizes();
    void storeTargetType();

    void initDefaultParameters();
    void findSensorType();

    status_t setPreviewSize(const CameraParameters& params);
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
    status_t setOrientation(const CameraParameters& params);
    status_t setLensshadeValue(const CameraParameters& params);
    status_t setISOValue(const CameraParameters& params);
    status_t setPictureFormat(const CameraParameters& params);
    status_t setSharpness(const CameraParameters& params);
    status_t setContrast(const CameraParameters& params);
    status_t setSaturation(const CameraParameters& params);
    status_t setSceneMode(const CameraParameters& params);

    void setGpsParameters();
    void storePreviewFrameForPostview();
    bool isValidDimension(int w, int h);

    Mutex mLock;
    Mutex mCamframeTimeoutLock;
    bool camframe_timeout_flag;
    bool mReleasedRecordingFrame;

    void receiveRawPicture(void);
    void receiveRawSnapshot(void);

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

    int mBrightness;
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
};

}; // namespace android

#endif
