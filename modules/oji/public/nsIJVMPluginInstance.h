/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

////////////////////////////////////////////////////////////////////////////////
// NETSCAPE JAVA VM PLUGIN EXTENSIONS
// 
// This interface allows a Java virtual machine to be plugged into
// Communicator to implement the APPLET tag and host applets.
// 
// Note that this is the C++ interface that the plugin sees. The browser
// uses a specific implementation of this, nsJVMPlugin, found in jvmmgr.h.
////////////////////////////////////////////////////////////////////////////////

#ifndef nsIJVMPluginInstance_h___
#define nsIJVMPluginInstance_h___

#ifndef nsISupports_h___
#include "nsISupports.h"
#endif
#ifndef JNI_H
#include "jni.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Java VM Plugin Instance Interface

#define NS_IJVMPLUGININSTANCE_IID                    \
{ /* a0c057d0-01c1-11d2-815b-006008119d7a */         \
    0xa0c057d0,                                      \
    0x01c1,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

class nsIJVMPluginInstance : public nsISupports {
public:
	NS_DEFINE_STATIC_IID_ACCESSOR(NS_IJVMPLUGININSTANCE_IID)
	
    // This method is called when LiveConnect wants to find the Java object
    // associated with this plugin instance, e.g. the Applet or JavaBean object.
    NS_IMETHOD
    GetJavaObject(jobject *result) = 0;

    NS_IMETHOD
    GetText(const char* *result) = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIJVMPluginInstance_h___ */
