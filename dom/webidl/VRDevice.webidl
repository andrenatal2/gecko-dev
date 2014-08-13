/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

enum VREye {
  "left",
  "right"
};

dictionary VRPoint3D {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

dictionary VRPoint4D {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double w = 0.0;
};

// We have DOMRect, but it's an interface?
// Come on..
dictionary VRRect {
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
};

dictionary VRFieldOfView {
  double upDegrees = 0.0;
  double rightDegrees = 0.0;
  double downDegrees = 0.0;
  double leftDegrees = 0.0;
};

dictionary VRPositionState {
  double timeStamp = 0.0;

  VRPoint3D position;
  VRPoint3D linearVelocity;
  VRPoint3D linearAcceleration;

  VRPoint4D orientation;
  VRPoint3D angularVelocity;
  VRPoint3D angularAcceleration;
};

[Pref="dom.vr.enabled"]
interface VRDevice {
  /**
   * An identifier for the distinct hardware unit that this
   * VR Device is a part of.  All VRDevice/Sensors that come
   * from the same hardware will have the same hardwareId
   */
  [Pure, Cached] readonly attribute DOMString hardwareUnitId;

  /**
   * An identifier for this distinct sensor/device on a physical
   * hardware device.  This shouldn't change across browser
   * restrats, allowing configuration data to be saved based on it.
   */
  [Pure, Cached] readonly attribute DOMString deviceId;

  /**
   * a device name, a user-readable name identifying it
   */
  [Pure, Cached] readonly attribute DOMString deviceName;
};

[Pref="dom.vr.enabled"]
interface HMDVRDevice : VRDevice {
  /* The translation that should be applied to the view matrix for rendering each eye */
  VRPoint3D getEyeTranslation(VREye whichEye);

  // the FOV that the HMD was configured with
  VRFieldOfView getCurrentEyeFieldOfView(VREye whichEye);

  // the recommended FOV, per eye.
  VRFieldOfView getRecommendedEyeFieldOfView(VREye whichEye);

  // the maximum FOV, per eye.  Above this, rendering will look broken.
  VRFieldOfView getMaximumEyeFieldOfView(VREye whichEye);

  // set a field of view
  void setFieldOfView(optional VRFieldOfView leftFOV,
                      optional VRFieldOfView rightFOV,
                      optional double zNear = 0.01,
                      optional double zFar = 10000.0);

  // return a recommended rect for this eye.  Only useful for Canvas rendering,
  // the x/y coordinates will be the location in the canvas where this eye should
  // begin, and the width/height are the dimensions.  Any canvas in the appropriate
  // ratio will work.
  VRRect getRecommendedEyeRenderRect(VREye whichEye);
};

[Pref="dom.vr.enabled"]
interface PositionSensorVRDevice : VRDevice {
  VRPositionState getState(optional double timeOffset = 0.0);

  // zero this sensor
  void zeroSensor();
};

/* Testing temporary APIs */
partial interface HMDVRDevice {
  // hack for testing
  void xxxToggleElementVR(Element element);
};
