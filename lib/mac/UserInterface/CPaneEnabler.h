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

// CPaneEnabler is a more general form of CButtonEnabler, originally written by John R. McMullen.
// The generalization was done by Mark Young.

// CSlaveEnabler was written by John R. McMullen

#pragma once

#include "LAttachment.h"

class LPane;

//======================================
class CPaneEnabler : public LAttachment
// This enables a pane based on FindCommandStatus (or another policy if the pane
// inherits from MPaneEnablerPolicy
//======================================
{
public:
	enum { class_ID = 'BtEn' }; // It used to be CButtonEnabler (thus the class_ID 'BtEn')
					CPaneEnabler(LStream* inStream);
					CPaneEnabler() ;					
	virtual			~CPaneEnabler();
	virtual	void	ExecuteSelf(MessageT inMessage, void *ioParam);
	static void		UpdatePanes();
protected:
	void			CommonInit ( ) ;
	
	LPane*			mPane;
}; // class CPaneEnabler

//======================================
class CSlaveEnabler
// This enables a pane based on another control with the same superview.  When the
// controlling control is on, this pane enables.  When the controlling control is off,
// this pane disables.  The controlling control is typically a checkbox or radio button.
//======================================
:	public LAttachment
,	public LListener
{
public:
	enum { class_ID = 'Slav' };
					CSlaveEnabler(LStream* inStream);
	virtual	void	ExecuteSelf(MessageT inMessage, void *ioParam);
	virtual	void	ListenToMessage(MessageT inMessage, void *ioParam);
protected:
	void			Update(SInt32 inValue);
protected:
	PaneIDT			mControllingID; // read from stream
	LPane* 			mPane;
	LControl*		mControllingControl;
}; // class CPaneEnabler

