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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*

  An RDF-specific content sink. The content sink is targeted by the
  parser for building the RDF content model.

 */

#ifndef nsIRDFContentSink_h___
#define nsIRDFContentSink_h___

#include "nsIXMLContentSink.h"
class nsIDocument;
class nsIRDFDataSource;
class nsINameSpaceManager;
class nsIURI;

// {751843E2-8309-11d2-8EAC-00805F29F370}
#define NS_IRDFCONTENTSINK_IID \
{ 0x751843e2, 0x8309, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

/**
 * This interface represents a content sink for RDF files.
 */

class nsIRDFContentSink : public nsIXMLContentSink {
public:
    static const nsIID& GetIID() { static nsIID iid = NS_IRDFCONTENTSINK_IID; return iid; }

    /**
     * Initialize the content sink.
     */
    NS_IMETHOD Init(nsIURI* aURL, nsINameSpaceManager* aNameSpaceManager) = 0;

    /**
     * Set the content sink's RDF Data source
     */
    NS_IMETHOD SetDataSource(nsIRDFDataSource* aDataSource) = 0;

    /**
     * Retrieve the content sink's RDF data source.
     */
    NS_IMETHOD GetDataSource(nsIRDFDataSource*& rDataSource) = 0;
};


/**
 * This constructs a content sink that can be used without a
 * document, say, to create a stand-alone in-memory graph.
 */
nsresult
NS_NewRDFContentSink(nsIRDFContentSink** aResult);

#endif // nsIRDFContentSink_h___
