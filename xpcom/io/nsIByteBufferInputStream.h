/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsIByteBufferInputStream_h___
#define nsIByteBufferInputStream_h___

#include "nsIInputStream.h"
#include "nsIOutputStream.h"

#define NS_IBYTEBUFFERINPUTSTREAM_IID                \
{ /* 40767100-eb9c-11d2-931c-00104ba0fd40 */         \
    0x40767100,                                      \
    0xeb9c,                                          \
    0x11d2,                                          \
    {0x93, 0x1c, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

#define NS_BYTEBUFFERINPUTSTREAM_CID                 \
{ /* d713ada0-0d6b-11d3-9331-00104ba0fd40 */         \
    0xd713ada0,                                      \
    0x0d6b,                                          \
    0x11d3,                                          \
    {0x93, 0x31, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

class nsIByteBufferInputStream : public nsIInputStream {
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IBYTEBUFFERINPUTSTREAM_IID);

    NS_IMETHOD Fill(nsIInputStream* stream, PRUint32 aCount, PRUint32 *aWriteCount) = 0;

    NS_IMETHOD Fill(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount) = 0;

};

////////////////////////////////////////////////////////////////////////////////

extern NS_COM nsresult
NS_NewByteBufferInputStream(nsIByteBufferInputStream* *result,
                            PRBool blocking = PR_FALSE,
                            PRUint32 size = 4096);

extern NS_COM nsresult
NS_NewPipe(nsIInputStream* *inStrResult,
           nsIOutputStream* *outStrResult,
           PRBool blocking = PR_TRUE,
           PRUint32 bufferSize = 4096);

////////////////////////////////////////////////////////////////////////////////

#endif /* nsByteBufferInputStream_h___ */
