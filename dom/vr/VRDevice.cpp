/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsWrapperCache.h"

#include "mozilla/dom/Element.h"
#include "mozilla/dom/VRDeviceBinding.h"
#include "mozilla/dom/ElementBinding.h"
#include "VRDevice.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace gfx {
namespace vr {
class HMDInfo;
}
}

namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(VRDevice)
NS_IMPL_CYCLE_COLLECTING_RELEASE(VRDevice)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(VRDevice)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(VRDevice, mParent)

/* virtual */ JSObject*
VRDevice::WrapObject(JSContext* aCx)
{
  return VRDeviceBinding::Wrap(aCx, this);
}

/* virtual */ JSObject*
HMDVRDevice::WrapObject(JSContext* aCx)
{
  return HMDVRDeviceBinding::Wrap(aCx, this);
}

/* virtual */ JSObject*
PositionSensorVRDevice::WrapObject(JSContext* aCx)
{
  return PositionSensorVRDeviceBinding::Wrap(aCx, this);
}

static void
ReleaseHMDInfoRef(void *, nsIAtom*, void *aPropertyValue, void *)
{
  if (aPropertyValue) {
    static_cast<vr::HMDInfo*>(aPropertyValue)->Release();
  }
}

void
HMDVRDevice::XxxToggleElementVR(Element& aElement)
{
  vr::HMDInfo* hmdPtr = static_cast<vr::HMDInfo*>(aElement.GetProperty(nsGkAtoms::vr_state));
  if (hmdPtr) {
    aElement.DeleteProperty(nsGkAtoms::vr_state);
    return;
  }

  RefPtr<vr::HMDInfo> hmdRef = mHMD;
  aElement.SetProperty(nsGkAtoms::vr_state, hmdRef.forget().drop(),
                       ReleaseHMDInfoRef,
                       true);
}

} // namespace dom
} // namespace mozilla
