/* 
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are Copyright (C) 1999 Sun Microsystems,
 * Inc. All Rights Reserved. 
 */
#include "PlugletStreamListener.h"
#include "PlugletEngine.h"



jmethodID PlugletStreamListener::onStartBindingMID = NULL;
jmethodID PlugletStreamListener::onDataAvailableMID = NULL;
jmethodID PlugletStreamListener::onFileAvailableMID = NULL;
jmethodID PlugletStreamListener::onStopBindingMID = NULL;
jmethodID PlugletStreamListener::getStreamTypeMID = NULL;

void PlugletStreamListener::Initialize(void) {
    JNIEnv * env =  PlugletEngine::GetJNIEnv();
    jclass clazz = env->FindClass("org/mozilla/pluglet/PlugletStreamListener");
    onStartBindingMID = env->GetMethodID(clazz, "onStartBinding","(Lorg/mozilla/pluglet/mozilla/PlugletStreamInfo;)V");
    onDataAvailableMID = env->GetMethodID(clazz,"onDataAvailable","(Lorg/mozilla/pluglet/mozilla/PlugletStreamInfo;Ljava/io/InputStream;I)V");
    onFileAvailableMID = env->GetMethodID(clazz,"onFileAvailable","(Lorg/mozilla/pluglet/mozilla/PlugletStreamInfo;Ljava/lang/String;)V");
    getStreamTypeMID = env->GetMethodID(clazz,"getStreamType","()I");
    onStopBindingMID = env->GetMethodID(clazz,"onStopBinding","(Lorg/mozilla/pluglet/mozilla/PlugletStreamInfo;I)V");
}

PlugletStreamListener::PlugletStreamListener(jobject object) {
    NS_INIT_REFCNT();
    jthis = PlugletEngine::GetJNIEnv()->NewGlobalRef(object);
    if (!onStopBindingMID) {
	Initialize();
    }
}

PlugletStreamListener::~PlugletStreamListener(void) {
    PlugletEngine::GetJNIEnv()->DeleteGlobalRef(jthis);
}

NS_METHOD  PlugletStreamListener::OnStartBinding(nsIPluginStreamInfo* pluginInfo) {
    JNIEnv * env = PlugletEngine::GetJNIEnv();
    //nb env->CallVoidMethod(jthis,onStartBindingMID,NewPluginStreamInfo(Plugin::env,pluginInfo));
    return NS_OK;
}
    
NS_METHOD PlugletStreamListener::OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length) {
    JNIEnv * env = PlugletEngine::GetJNIEnv();   
    //nb env->CallVoidMethod(jthis,onDataAvailableMID,NewPluginStreamInfo(Plugin::env,pluginInfo),
    //				NewInputStream(Plugin::env,input),(jint)length);
    return NS_OK;
}

NS_METHOD PlugletStreamListener::OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName) {
    JNIEnv * env = PlugletEngine::GetJNIEnv();
    //nb Plugin::env->CallVoidMethod(jthis,onFileAvailableMID,NewPluginStreamInfo(Plugin::env,pluginInfo),
    //		       Plugin::env->NewStringUTF(fileName));
    return NS_OK;
}

NS_METHOD PlugletStreamListener::OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status) {
    JNIEnv * env = PlugletEngine::GetJNIEnv();
    //nb env->CallVoidMethod(jthis,onStopBindingMID,NewPluginStreamInfo(Plugin::env,pluginInfo),status);
    return NS_OK;
}
NS_METHOD PlugletStreamListener::GetStreamType(nsPluginStreamType *result) {
    JNIEnv * env = PlugletEngine::GetJNIEnv();
    *result = (nsPluginStreamType)env->CallIntMethod(jthis,getStreamTypeMID);
    return NS_OK;
}

static NS_DEFINE_IID(kIPluginStreamListenerIID, NS_IPLUGINSTREAMLISTENER_IID);

NS_IMPL_ISUPPORTS(PlugletStreamListener, kIPluginStreamListenerIID);





