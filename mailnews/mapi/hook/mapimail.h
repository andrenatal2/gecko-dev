/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef _MAPI_MAIL_H_
#define _MAPI_MAIL_H_

#include "nscpmapi.h"
#include <structs.h>    // for MWContext

//extern "C" {

//
// This function will create a composition window and either do
// a blind send or pop up the compose window for the user to 
// complete the operation
//
// Return: appropriate MAPI return code...
//
//
extern "C" LONG
DoFullMAPIMailOperation(MAPISendMailType      *sendMailPtr,
											  const char            *pInitialText,
                        BOOL                  winShowFlag);

//
// This function will create a composition window and just attach
// the attachments of interest and pop up the window...
//
// Return: appropriate MAPI return code...
//
//
extern "C" LONG
DoPartialMAPIMailOperation(MAPISendDocumentsType *sendDocPtr);

//
// This function will save a message into the Communicator "Drafts"
// folder with no UI showing.
//
// Return: appropriate MAPI return code...
//
//
extern "C" LONG 
DoMAPISaveMailOperation(MAPISendMailType      *sendMailPtr,
											  const char            *pInitialText);

//
// This will fire off a "get mail in background operation" in an
// async. fashion.
//
extern "C" void
MAPIGetNewMessagesInBackground(void);


// } // extern "C"

#endif // _MAPI_MAIL_H_
