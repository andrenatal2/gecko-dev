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

#pragma once

#include <LGACaption.h>
#include <LBroadcaster.h>


//
// CChameleonCaption (should be renamed to CColorCaption)
//
// A caption that can have its text color set dynamically (not just by
// text traits).
//

class CChameleonCaption : public LGACaption
{
public:
	enum { class_ID = 'ccpt' };
	
	CChameleonCaption ( LStream * inStream ) ;

	virtual void	SetColor ( RGBColor & textColor, RGBColor & backColor ) ;
	virtual void	SetColor ( RGBColor & textColor ) ;
	
protected:

	virtual void		DrawSelf();	
	virtual void		DrawText(Rect frame, Int16 inJust);	

	RGBColor			mTextColor;
	RGBColor			mBackColor;

}; // CChameleonCaption


//
// CChameleonBroadcastCaption (should be renamed to CColorCaption)
//
// A color-changing caption that broadcasts when you click on it. It will also
// change text traits on roll-over (disable by passing the same thing for both).
//

class CChameleonBroadcastCaption : public CChameleonCaption, public LBroadcaster
{
public:
	enum { class_ID = 'ccbc' } ;

	CChameleonBroadcastCaption ( LStream * inStream ) ;
	
private:
	
	virtual void ClickSelf ( const SMouseDownEvent & inEvent ) ;
	virtual void MouseWithin ( Point inWhere, const EventRecord & inEvent ) ;
	virtual void MouseLeave ( ) ;
	
	MessageT	mMessage;
	ResIDT		mRolloverTraits;
	ResIDT		mSavedTraits;

}; // class CChameleonBroadcastCaption