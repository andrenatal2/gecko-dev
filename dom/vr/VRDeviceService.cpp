/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/Hal.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"

#include "VRDeviceService.h"
#include "VRDevice.h"
#include "nsAutoPtr.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "GeneratedEvents.h"
#include "nsIDOMWindow.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"
#include "mozilla/dom/Promise.h"

#include "gfxVR.h"

using namespace mozilla;

namespace mozilla {
namespace dom {

namespace {

const char* kEnabledPref = "dom.vr.enabled";

void
CopyFieldOfView(const gfx::VRFieldOfView& aSrc, VRFieldOfView& aDest)
{
  aDest.mUpDegrees = aSrc.upDegrees;
  aDest.mRightDegrees = aSrc.rightDegrees;
  aDest.mDownDegrees = aSrc.downDegrees;
  aDest.mLeftDegrees = aSrc.leftDegrees;
}

gfx::VRHMDInfo::Eye
EyeToEye(const VREye& aEye)
{
  return aEye == VREye::Left ? gfx::VRHMDInfo::Eye_Left : gfx::VRHMDInfo::Eye_Right;
}

} // namespace

class HMDInfoVRDevice : public HMDVRDevice
{
public:
  HMDInfoVRDevice(gfx::VRHMDInfo* aHMD)
    : HMDVRDevice(nullptr, aHMD)
  {
    // XXX TODO use real names/IDs
    mHWID.AppendPrintf("HMDInfo-0x%llx", aHMD);
    mDeviceId.AssignLiteral("somedevid");
    mDeviceName.AssignLiteral("HMD Device");

    mValid = true;
  }

  virtual ~HMDInfoVRDevice() { }

  virtual void SetFieldOfView(const dom::VRFieldOfView& aLeftFOV,
                              const dom::VRFieldOfView& aRightFOV)
  {
    gfx::VRFieldOfView left = gfx::VRFieldOfView(aLeftFOV.mUpDegrees, aLeftFOV.mRightDegrees,
                                                 aLeftFOV.mDownDegrees, aLeftFOV.mLeftDegrees);
    gfx::VRFieldOfView right = gfx::VRFieldOfView(aRightFOV.mUpDegrees, aRightFOV.mRightDegrees,
                                                  aRightFOV.mDownDegrees, aRightFOV.mLeftDegrees);

    if (left.IsZero())
      left = mHMD->GetRecommendedEyeFOV(VRHMDInfo::Eye_Left);
    if (right.IsZero())
      right = mHMD->GetRecommendedEyeFOV(VRHMDInfo::Eye_Right);

    mHMD->SetFOV(left, right);
  }

  virtual void GetEyeTranslation(VREye aEye, VRPoint3D& aTranslationOut) {
    gfx::Point3D p = mHMD->GetEyeTranslation(EyeToEye(aEye));

    aTranslationOut.mX = p.x;
    aTranslationOut.mY = p.y;
    aTranslationOut.mZ = p.z;
  }

  virtual void GetCurrentEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) {
    CopyFieldOfView(mHMD->GetEyeFOV(EyeToEye(aEye)), aFOV);
  }

  virtual void GetRecommendedEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) {
    CopyFieldOfView(mHMD->GetRecommendedEyeFOV(EyeToEye(aEye)), aFOV);
  }

  virtual void GetMaximumEyeFieldOfView(VREye aEye, VRFieldOfView& aFOV) {
    CopyFieldOfView(mHMD->GetMaximumEyeFOV(EyeToEye(aEye)), aFOV);
  }
};

class HMDPositionVRDevice : public PositionSensorVRDevice
{
public:
  HMDPositionVRDevice(gfx::VRHMDInfo* aHMD)
    : PositionSensorVRDevice(nullptr)
    , mHMD(aHMD)
  {
    // XXX TODO use real names/IDs
    mHWID.AppendPrintf("HMDInfo-0x%llx", aHMD);
    mDeviceId.AssignLiteral("somedevid");
    mDeviceName.AssignLiteral("HMD Position Device");

    mValid = true;
  }

  void GetState(double timeOffset, VRPositionState& aOut) {
    gfx::VRHMDSensorState state = mHMD->GetSensorState(timeOffset);

    aOut.mTimeStamp = state.timestamp;

    if (state.flags & gfx::VRHMDInfo::State_Position) {
      aOut.mPosition.mX = state.position[0];
      aOut.mPosition.mY = state.position[1];
      aOut.mPosition.mZ = state.position[2];

      aOut.mLinearVelocity.mX = state.linearVelocity[0];
      aOut.mLinearVelocity.mY = state.linearVelocity[1];
      aOut.mLinearVelocity.mZ = state.linearVelocity[2];

      aOut.mLinearAcceleration.mX = state.linearAcceleration[0];
      aOut.mLinearAcceleration.mY = state.linearAcceleration[1];
      aOut.mLinearAcceleration.mZ = state.linearAcceleration[2];
    }

    if (state.flags & gfx::VRHMDInfo::State_Orientation) {
      aOut.mOrientation.mX = state.orientation[0];
      aOut.mOrientation.mY = state.orientation[1];
      aOut.mOrientation.mZ = state.orientation[2];
      aOut.mOrientation.mW = state.orientation[3];

      aOut.mAngularVelocity.mX = state.angularVelocity[0];
      aOut.mAngularVelocity.mY = state.angularVelocity[1];
      aOut.mAngularVelocity.mZ = state.angularVelocity[2];

      aOut.mAngularAcceleration.mX = state.angularAcceleration[0];
      aOut.mAngularAcceleration.mY = state.angularAcceleration[1];
      aOut.mAngularAcceleration.mZ = state.angularAcceleration[2];
    }
  }

protected:
  RefPtr<gfx::VRHMDInfo> mHMD;
};

NS_IMPL_ISUPPORTS(VRDeviceService, nsIObserver)

VRDeviceService::VRDeviceService()
  : mInitialized(false)
  , mShuttingDown(false)
{
  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  observerService->AddObserver(this,
                               NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                               false);
}

VRDeviceService::~VRDeviceService()
{
}

NS_IMETHODIMP
VRDeviceService::Observe(nsISupports* aSubject,
                         const char* aTopic,
                         const char16_t* aData)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  observerService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);

  BeginShutdown();
  return NS_OK;
}

/* static */
already_AddRefed<VRDeviceService>
VRDeviceService::Create()
{
  nsRefPtr<VRDeviceService> service = new VRDeviceService();
  return service.forget();
}

void
VRDeviceService::BeginShutdown()
{
  mKnownDevices.Clear();
}

bool
VRDeviceService::Initialize()
{
  if (mInitialized)
    return true;

  mKnownDevices.Clear();

  if (!gfx::VRHMDManagerOculus::Init()) {
    NS_WARNING("Failed to initialize Oculus HMD Manager");
    return false;
  }

  nsTArray<RefPtr<gfx::VRHMDInfo> > hmds;
  gfx::VRHMDManagerOculus::GetOculusHMDs(hmds);

  for (size_t i = 0; i < hmds.Length(); ++i) {
    uint32_t sensorBits = hmds[i]->GetSupportedSensorStateBits();
    mKnownDevices.AppendElement(new HMDInfoVRDevice(hmds[i]));

    if (sensorBits &
        (gfx::VRHMDInfo::State_Position | gfx::VRHMDInfo::State_Orientation))
    {
      mKnownDevices.AppendElement(new HMDPositionVRDevice(hmds[i]));
    }

    // XXX we should have a more logical place to start & stop tracking
    hmds[i]->StartSensorTracking();
  }

  mInitialized = true;
  return true;
}

void
VRDeviceService::GetVRDevices(Promise* aPromise)
{
  if (!Initialize()) {
    NS_WARNING("VRDeviceService: Initialization failed");
    aPromise->MaybeReject(NS_ERROR_FAILURE);
    return;
  }

  aPromise->MaybeResolve(mKnownDevices);
}

} // namespace dom
} // namespace mozilla
