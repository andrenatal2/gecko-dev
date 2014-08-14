/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_VRDevice_h_
#define mozilla_dom_VRDevice_h_

#include <stdint.h>

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/VRDeviceBinding.h"
#include "gfxVR.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class Element;

class VRDevice : public nsISupports,
                 public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(VRDevice)

  void GetHardwareUnitId(nsAString& aHWID) const { aHWID = mHWID; }
  void GetDeviceId(nsAString& aDeviceId) const { aDeviceId = mDeviceId; }
  void GetDeviceName(nsAString& aDeviceName) const { aDeviceName = mDeviceName; }

  bool IsValid() { return mValid; }

  virtual void Shutdown() { }

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

protected:
  VRDevice(nsISupports* aParent)
    : mParent(aParent)
    , mValid(false)
  {
    SetIsDOMBinding();

    mHWID.AssignLiteral("uknown");
    mDeviceId.AssignLiteral("unknown");
    mDeviceName.AssignLiteral("unknown");
  }

  virtual ~VRDevice() {
    Shutdown();
  }

  nsCOMPtr<nsISupports> mParent;
  nsString mHWID;
  nsString mDeviceId;
  nsString mDeviceName;

  bool mValid;
};

class HMDVRDevice : public VRDevice
{
public:
  virtual void GetEyeTranslation(VREye aEye, VRPoint3D& aTranslationOut) = 0;

  virtual void SetFieldOfView(const VRFieldOfView& aLeftFOV, const VRFieldOfView& aRightFOV,
                              double zNear, double zFar) = 0;
  virtual void GetCurrentEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) = 0;
  virtual void GetRecommendedEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) = 0;
  virtual void GetMaximumEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) = 0;
  virtual void GetRecommendedEyeRenderRect(VREye aEye, VRRect& aRect) = 0;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void XxxToggleElementVR(Element& aElement);

  gfx::VRHMDInfo *GetHMD() { return mHMD.get(); }

protected:
  HMDVRDevice(nsISupports* aParent, gfx::VRHMDInfo* aHMD)
    : VRDevice(aParent)
    , mHMD(aHMD)
  { }

  virtual ~HMDVRDevice() { }

  RefPtr<gfx::VRHMDInfo> mHMD;
};

class PositionSensorVRDevice : public VRDevice
{
public:
  virtual void GetState(double timeOffset, VRPositionState& aOut) = 0;

  virtual void ZeroSensor() = 0;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

protected:
  PositionSensorVRDevice(nsISupports* aParent)
    : VRDevice(aParent)
  { }

  virtual ~PositionSensorVRDevice() { }
};

} // namespace dom
} // namespace mozilla

#endif
