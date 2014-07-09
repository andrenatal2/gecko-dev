/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This file contains just the needed struct definitions for
 * interacting with the Oculus VR C API, without needing to #include
 * OVR_CAPI.h directly.  Note that it uses the same type names as the
 * CAPI, and cannot be #included at the same time as OVR_CAPI.h.  It
 * does not include the entire C API, just want's needed.
 */

#ifdef OVR_CAPI_h
#warning OVR_CAPI.h included before ovr_capi_dynamic.h, skpping this
#define mozilla_ovr_capi_dynamic_h_
#endif

#ifndef mozilla_ovr_capi_dynamic_h_
#define mozilla_ovr_capi_dynamic_h_

#define OVR_CAPI_LIMITED_MOZILLA 1

#ifdef __cplusplus 
extern "C" {
#endif

typedef char ovrBool;
typedef struct { int x, y; } ovrVector2i;
typedef struct { int w, h; } ovrSizei;
typedef struct { ovrVector2i Pos; ovrSizei Size; } ovrRecti;
typedef struct { float x, y, z, w; } ovrQuatf;
typedef struct { float x, y; } ovrVector2f;
typedef struct { float x, y, z; } ovrVector3f;
typedef struct { float M[4][4]; } ovrMatrix4f;

typedef struct {
  ovrQuatf Orientation;
  ovrVector3f Position;
} ovrPosef;

typedef struct {
  ovrPosef Pose;
  ovrVector3f AngularVelocity;
  ovrVector3f LinearVelocity;
  ovrVector3f AngularAcceleration;
  ovrVector3f LinearAcceleration;
  double TimeInSeconds;
} ovrPoseStatef;

typedef struct {
  float UpTan;
  float DownTan;
  float LeftTan;
  float RightTan;
} ovrFovPort;

typedef enum {
  ovrHmd_None             = 0,    
  ovrHmd_DK1              = 3,
  ovrHmd_DKHD             = 4,
  ovrHmd_CrystalCoveProto = 5,
  ovrHmd_DK2              = 6,
  ovrHmd_Other
} ovrHmdType;

typedef enum {
  ovrHmdCap_Present           = 0x0001,
  ovrHmdCap_Available         = 0x0002,
  ovrHmdCap_Orientation       = 0x0010,
  ovrHmdCap_LowPersistence    = 0x0080,
  ovrHmdCap_LatencyTest       = 0x0100,
  ovrHmdCap_DynamicPrediction = 0x0200,
  ovrHmdCap_NoVSync           = 0x1000,
  ovrHmdCap_NoRestore         = 0x4000,
} ovrHmdCapBits;

typedef enum {
  ovrSensorCap_Orientation = 0x0010,
  ovrSensorCap_YawCorrection = 0x0020,
  ovrSensorCap_Position = 0x0040
} ovrSensorCaps;

typedef enum {
  ovrDistortionCap_Chromatic = 0x01,
  ovrDistortionCap_TimeWarp  = 0x02,
  ovrDistortionCap_Vignette  = 0x08
} ovrDistortionCaps;

typedef enum {
  ovrEye_Left  = 0,
  ovrEye_Right = 1,
  ovrEye_Count = 2
} ovrEyeType;

typedef struct ovrHmdStruct* ovrHmd;

typedef struct ovrHmdDesc_ {
  ovrHmd      Handle;
  ovrHmdType  Type;
  const char* ProductName;    
  const char* Manufacturer;
  unsigned int HmdCaps;
  unsigned int SensorCaps;
  unsigned int DistortionCaps;
  ovrSizei    Resolution;
  ovrVector2i WindowsPos;     
  ovrFovPort  DefaultEyeFov[ovrEye_Count];
  ovrFovPort  MaxEyeFov[ovrEye_Count];
  ovrEyeType  EyeRenderOrder[ovrEye_Count];
  const char* DisplayDeviceName;
  int        DisplayId;
} ovrHmdDesc;

typedef enum {
  ovrStatus_OrientationTracked    = 0x0001,
  ovrStatus_PositionTracked       = 0x0002,
  ovrStatus_PositionConnected     = 0x0020,
  ovrStatus_HmdConnected          = 0x0080
} ovrStatusBits;

typedef struct ovrSensorState_ {
  ovrPoseStatef  Predicted;
  ovrPoseStatef  Recorded;
  float          Temperature;    
  unsigned int   StatusFlags;
} ovrSensorState;

typedef struct ovrSensorDesc_ {
  short   VendorId;
  short   ProductId;
  char    SerialNumber[24];
} ovrSensorDesc;

typedef struct ovrFrameTiming_ {
  float DeltaSeconds;
  double ThisFrameSeconds;
  double TimewarpPointSeconds;
  double NextFrameSeconds;
  double ScanoutMidpointSeconds;
  double EyeScanoutSeconds[2];    
} ovrFrameTiming;

typedef struct ovrEyeRenderDesc_ {
  ovrEyeType  Eye;        
  ovrFovPort  Fov;
  ovrRecti DistortedViewport;
  ovrVector2f PixelsPerTanAngleAtCenter;
  ovrVector3f ViewAdjust;
} ovrEyeRenderDesc;

typedef struct ovrDistortionVertex_ {
  ovrVector2f Pos;
  float       TimeWarpFactor;
  float       VignetteFactor;
  ovrVector2f TexR;
  ovrVector2f TexG;
  ovrVector2f TexB;    
} ovrDistortionVertex;

typedef struct ovrDistortionMesh_ {
  ovrDistortionVertex* pVertexData;
  unsigned short*      pIndexData;
  unsigned int         VertexCount;
  unsigned int         IndexCount;
} ovrDistortionMesh;

typedef ovrBool (*pfn_ovr_Initialize)();
typedef void (*pfn_ovr_Shutdown)();
typedef int (*pfn_ovrHmd_Detect)();
typedef ovrHmd (*pfn_ovrHmd_Create)(int index);
typedef void (*pfn_ovrHmd_Destroy)(ovrHmd hmd);
typedef ovrHmd (*pfn_ovrHmd_CreateDebug)(ovrHmdType type);
typedef const char* (*pfn_ovrHmd_GetLastError)(ovrHmd hmd);
typedef ovrBool (*pfn_ovrHmd_StartSensor)(ovrHmd hmd, unsigned int supportedCaps, unsigned int requiredCaps);
typedef void (*pfn_ovrHmd_StopSensor)(ovrHmd hmd);
typedef void (*pfn_ovrHmd_ResetSensor)(ovrHmd hmd);
typedef ovrSensorState (*pfn_ovrHmd_GetSensorState)(ovrHmd hmd, double absTime);
typedef ovrBool (*pfn_ovrHmd_GetSensorDesc)(ovrHmd hmd, ovrSensorDesc* descOut);
typedef void (*pfn_ovrHmd_GetDesc)(ovrHmd hmd, ovrHmdDesc* desc);
typedef ovrSizei (*pfn_ovrHmd_GetFovTextureSize)(ovrHmd hmd, ovrEyeType eye, ovrFovPort fov, float pixelsPerDisplayPixel);
typedef ovrEyeRenderDesc (*pfn_ovrHmd_GetRenderDesc)(ovrHmd hmd, ovrEyeType eyeType, ovrFovPort fov);
typedef ovrBool (*pfn_ovrHmd_CreateDistortionMesh)(ovrHmd hmd, ovrEyeType eyeType, ovrFovPort fov, unsigned int distortionCaps, ovrDistortionMesh *meshData);
typedef void (*pfn_ovrHmd_DestroyDistortionMesh)(ovrDistortionMesh* meshData);
typedef void (*pfn_ovrHmd_GetRenderScaleAndOffset)(ovrFovPort fov, ovrSizei textureSize, ovrRecti renderViewport, ovrVector2f uvScaleOffsetOut[2]);
typedef ovrFrameTiming (*pfn_ovrHmd_GetFrameTiming)(ovrHmd hmd, unsigned int frameIndex);
typedef ovrFrameTiming (*pfn_ovrHmd_BeginFrameTiming)(ovrHmd hmd, unsigned int frameIndex);
typedef void (*pfn_ovrHmd_EndFrameTiming)(ovrHmd hmd);
typedef void (*pfn_ovrHmd_ResetFrameTiming)(ovrHmd hmd, unsigned int frameIndex, bool vsync);
typedef ovrPosef (*pfn_ovrHmd_GetEyePose)(ovrHmd hmd, ovrEyeType eye);
typedef void (*pfn_ovrHmd_GetEyeTimewarpMatrices)(ovrHmd hmd, ovrEyeType eye, ovrPosef renderPose, ovrMatrix4f twmOut[2]);
typedef double (*pfn_ovr_GetTimeInSeconds)();

#ifdef __cplusplus 
}
#endif

#endif /* mozilla_ovr_capi_dynamic_h_ */
