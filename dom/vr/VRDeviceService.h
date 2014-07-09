/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_VRDeviceService_h_
#define mozilla_dom_VRDeviceService_h_

#include <stdint.h>
#include "VRDevice.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsGlobalWindow.h"
#include "nsIFocusManager.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsTArray.h"

class nsIDOMDocument;

namespace mozilla {
namespace dom {

class Promise;

class VRDeviceService : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  // Create a service, generally one per window
  static already_AddRefed<VRDeviceService> Create();

  // The Navigator interface callback
  void GetVRDevices(Promise* aPromise);

  void BeginShutdown();

  bool Initialize();

protected:
  VRDeviceService();
  virtual ~VRDeviceService();

  bool mInitialized;
  nsTArray<nsRefPtr<VRDevice> > mKnownDevices;

  // true when shutdown has begun
  bool mShuttingDown;
};

} // namespace dom
} // namespace mozilla

#endif /* mozilla_dom_VRDeviceService_h_ */

