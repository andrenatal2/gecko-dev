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

/********************************************************************************************************
 
     Wrapper class for various parsing URL parsing routines...
 
*********************************************************************************************************/

#ifndef _nsMimeURLUtils_h__
#define _nsMimeURLUtils_h__

#include "nsIMimeURLUtils.h" 

class nsMimeURLUtils: public nsIMimeURLUtils { 
public: 
    nsMimeURLUtils();
    virtual ~nsMimeURLUtils();

    /* this macro defines QueryInterface, AddRef and Release for this class */
	  NS_DECL_ISUPPORTS

    static const nsIID& GetIID(void) { static nsIID iid = NS_IMIMEURLUTILS_IID; return iid; }

    NS_IMETHOD    URLType(const char *URL, PRInt32  *retType);

    NS_IMETHOD    ReduceURL (char *url, char **retURL);

    NS_IMETHOD    ScanForURLs(const char *input, PRInt32 input_size,
                              char *output, PRInt32 output_size, PRBool urls_only);

    NS_IMETHOD    MakeAbsoluteURL(char * absolute_url, const char * relative_url, char **retURL);

    NS_IMETHOD    ScanHTMLForURLs(const char* input, char **retBuf);

    NS_IMETHOD    ParseURL(const char *url, PRInt32 parts_requested, char **returnVal);
}; 

#endif /* _nsMimeURLUtils_h__ */
