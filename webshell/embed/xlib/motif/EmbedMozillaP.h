/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef __XmEmbedMozillaP_h__
#define __XmEmbedMozillaP_h__

#ifdef	__cplusplus
extern "C" {
#endif

#include "EmbedMozilla.h"
#include <Xm/ManagerP.h>

typedef struct
{
  int something;
} XmEmbedMozillaClassPart;

typedef struct _XmEmbedMozillaClassRec
{
  CoreClassPart	core_class;
  CompositeClassPart		composite_class;
  ConstraintClassPart		constraint_class;
  XmManagerClassPart		manager_class;
  XmEmbedMozillaClassPart	embed_mozilla_class;
} XmEmbedMozillaClassRec;

extern XmEmbedMozillaClassRec xmEmbedMozillaClassRec;

typedef struct 
{
  XtCallbackList                input_callback;
  Window                        embed_window;
} XmEmbedMozillaPart;

typedef struct _XmEmbedMozillaRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    XmEmbedMozillaPart		embed_mozilla;
} XmEmbedMozillaRec;

#ifdef	__cplusplus
}
#endif

#endif /* __XmEmbedMozillaP_h__ */
