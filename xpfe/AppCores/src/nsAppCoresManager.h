/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsAppCoresManager_h___
#define nsAppCoresManager_h___

#include "nscore.h"
#include "nsString.h"
#include "nsISupports.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMAppCoresManager.h"
#include "prio.h"
#include "nsVoidArray.h"

class nsIScriptContext;
class nsIDOMBaseAppCore;

////////////////////////////////////////////////////////////////////////////////
// nsAppCoresManager:
////////////////////////////////////////////////////////////////////////////////
class nsAppCoresManager : public nsIScriptObjectOwner, public nsIDOMAppCoresManager
{
  public:

    nsAppCoresManager();
    virtual ~nsAppCoresManager();
       
    NS_DECL_ISUPPORTS

    NS_IMETHOD    GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
    NS_IMETHOD    SetScriptObject(void* aScriptObject);

    NS_IMETHOD    Startup();

    NS_IMETHOD    Shutdown();

    NS_IMETHOD    Add(nsIDOMBaseAppCore* aAppcore);

    NS_IMETHOD    Remove(nsIDOMBaseAppCore* aAppcore);

    NS_IMETHOD    Find(const nsString& aId, nsIDOMBaseAppCore** aReturn);

  private:
    void        *mScriptObject;
    static nsVoidArray  mList;

        
};

#endif // nsAppCoresManager_h___
