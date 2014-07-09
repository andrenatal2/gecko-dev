/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_VR_H
#define GFX_VR_H

#include "mozilla/RefPtr.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/EnumeratedArray.h"

namespace mozilla {
namespace gfx {
namespace vr {

MOZ_BEGIN_ENUM_CLASS(HMDType, uint16_t)
  Oculus,
  NumHMDTypes
MOZ_END_ENUM_CLASS(HMDType)

struct FieldOfView {
  static FieldOfView FromCSSPerspectiveInfo(double aPerspectiveDistance,
                                            const Point& aPerspectiveOrigin,
                                            const Point& aTransformOrigin,
                                            const Rect& aContentRectangle)
  {
    /**/
    return FieldOfView();
  }

  FieldOfView() {}
  FieldOfView(double up, double right, double down, double left)
    : upDegrees(up), rightDegrees(right), downDegrees(down), leftDegrees(left)
  {}

  bool operator==(const FieldOfView& other) const {
    return other.upDegrees == upDegrees &&
           other.downDegrees == downDegrees &&
           other.rightDegrees == rightDegrees &&
           other.leftDegrees == leftDegrees;
  }

  bool operator!=(const FieldOfView& other) const {
    return !(*this == other);
  }

  bool IsZero() const {
    return upDegrees == 0.0 ||
      rightDegrees == 0.0 ||
      downDegrees == 0.0 ||
      leftDegrees == 0.0;
  }

  double upDegrees;
  double rightDegrees;
  double downDegrees;
  double leftDegrees;
};

// 12 floats per vertex. Position, tex coordinates
// for each channel, and 4 generic attributes
struct DistortionConstants {
  float eyeToSourceScaleAndOffset[4];
  float destinationScaleAndOffset[4];
};

struct DistortionVertex {
  float pos[2];
  float texR[2];
  float texG[2];
  float texB[2];
  float genericAttribs[4];
};

struct DistortionMesh {
  nsTArray<DistortionVertex> mVertices;
  nsTArray<uint16_t> mIndices;
};

struct HMDSensorState {
  double timestamp;
  uint32_t flags;
  float orientation[4];
  float position[3];
  float angularVelocity[3];
  float angularAcceleration[3];
  float linearVelocity[3];
  float linearAcceleration[3];

  void Clear() {
    memset(this, 0, sizeof(HMDSensorState));
  }
};

/* A pure data struct that can be used to see if
 * the configuration of one HMDInfo matches another; for rendering purposes,
 * really asking "can the rendering details of this one be used for the other"
 */
struct HMDConfiguration {
  HMDConfiguration() : hmdType(HMDType::NumHMDTypes) {}

  bool operator==(const HMDConfiguration& other) const {
    return hmdType == other.hmdType &&
      value == other.value &&
      fov[0] == other.fov[0] &&
      fov[1] == other.fov[1];
  }

  bool operator!=(const HMDConfiguration& other) const {
    return hmdType != other.hmdType ||
      value != other.value ||
      fov[0] != other.fov[0] ||
      fov[1] != other.fov[1];
  }

  bool IsValid() const {
    return hmdType != HMDType::NumHMDTypes;
  }

  HMDType hmdType;
  uint32_t value;
  FieldOfView fov[2];
};

class HMDInfo : public RefCounted<HMDInfo> {
public:
  enum Eye {
    Eye_Left,
    Eye_Right,
    NumEyes
  };

  enum StateValidFlags {
    State_Position = 1 << 1,
    State_Orientation = 1 << 2
  };

public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(HMDInfo)

  virtual ~HMDInfo() { }

  HMDType GetType() const { return mType; }

  virtual const FieldOfView& GetRecommendedEyeFOV(uint32_t whichEye) { return mRecommendedEyeFOV[whichEye]; }
  virtual const FieldOfView& GetMaximumEyeFOV(uint32_t whichEye) { return mMaximumEyeFOV[whichEye]; }

  const HMDConfiguration& GetConfiguration() const { return mConfiguration; }

  /* set the FOV for this HMD unit; this triggers a computation of all the remaining bits.  Returns false if it fails */
  virtual bool SetFOV(const FieldOfView& aFOVLeft, const FieldOfView& aFOVRight) = 0;
  const FieldOfView& GetEyeFOV(uint32_t whichEye)  { return mEyeFOV[whichEye]; }

  /* Suggested resolution for rendering a single eye.  Assumption is that side-by-side left/right rendering will be 2x of this size. */
  const IntSize& SuggestedEyeResolution() const { return mEyeResolution; }
  const Point3D& GetEyeTranslation(uint32_t whichEye) const { return mEyeTranslation[whichEye]; }

  virtual uint32_t GetSupportedSensorStateBits() { return mSupportedSensorBits; }
  virtual bool StartSensorTracking() = 0;
  virtual HMDSensorState GetSensorState(double timeOffset = 0.0) = 0;
  virtual void StopSensorTracking() = 0;

  virtual void FillDistortionConstants(uint32_t whichEye,
                                       const IntSize& textureSize, // the full size of the texture
                                       const IntRect& eyeViewport, // the viewport within the texture for the current eye
                                       const Size& destViewport,   // the size of the destination viewport
                                       const Rect& destRect,       // the rectangle within the dest viewport that this should be rendered
                                       DistortionConstants& values) = 0;

  virtual const DistortionMesh& GetDistortionMesh(uint32_t whichEye) const { return mDistortionMesh[whichEye]; }

  virtual bool GetReferenceScreenRect(IntRect& aRect) {
    if (mHasScreenRect)
      aRect = mScreenRect;
    return mHasScreenRect;
  }

protected:
  HMDInfo(HMDType aType) : mType(aType), mHasScreenRect(false) {}

  HMDType mType;
  HMDConfiguration mConfiguration;

  FieldOfView mEyeFOV[NumEyes];
  IntSize mEyeResolution;
  Point3D mEyeTranslation[NumEyes];
  DistortionMesh mDistortionMesh[NumEyes];
  uint32_t mSupportedSensorBits;

  FieldOfView mRecommendedEyeFOV[NumEyes];
  FieldOfView mMaximumEyeFOV[NumEyes];

  bool mHasScreenRect;
  IntRect mScreenRect;
};

class OculusHMDManager {
public:
  static bool Init();
  static void Destroy();
  static void GetOculusHMDs(nsTArray<RefPtr<HMDInfo> >& aHMDResult);
};

} // namespace vr
} // namespace gfx
} // namespace mozilla

#endif /* GFX_VR_H */
