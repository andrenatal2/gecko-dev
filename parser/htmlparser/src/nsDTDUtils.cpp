/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 

#include "nsDTDUtils.h"
#include "CNavDTD.h" 
#include "nsIParserNode.h"

#include "nsIObserverService.h"
#include "nsIServiceManager.h"

MOZ_DECL_CTOR_COUNTER(nsEntryStack);
MOZ_DECL_CTOR_COUNTER(nsDTDContext);
MOZ_DECL_CTOR_COUNTER(CTokenRecycler);
MOZ_DECL_CTOR_COUNTER(CObserverService);
 
/***************************************************************
  First, define the tagstack class
 ***************************************************************/


/**
 * Default constructor
 * @update	harishd 04/04/99
 * @update  gess 04/22/99
 */
nsEntryStack::nsEntryStack()  {
  
  MOZ_COUNT_CTOR(nsEntryStack);

  mCapacity=0;
  mCount=0;
  mEntries=0;
}

/**
 * Default destructor
 * @update  harishd 04/04/99
 * @update  gess 04/22/99
 */
nsEntryStack::~nsEntryStack() {

  MOZ_COUNT_DTOR(nsEntryStack);

  if(mEntries)
    delete [] mEntries;
  mCount=mCapacity=0;
  mEntries=0;
}

/**
 * Resets state of stack to be empty.
 * @update harishd 04/04/99
 */
void nsEntryStack::Empty(void) {
  mCount=0;
}

/**
 * 
 * @update  gess 04/22/99
 */
void nsEntryStack::Push(eHTMLTags aTag) {
  if(mCount==mCapacity){ 
    nsTagEntry* temp=new nsTagEntry[mCapacity+=50]; 
    if(temp){ 
      PRUint32 index=0; 
      for(index=0;index<mCount;index++) {
        temp[index]=mEntries[index];
      }
      delete [] mEntries;
      mEntries=temp;
    }
    else{
      //XXX HACK! This is very bad! We failed to get memory.
    }
  }
  mEntries[mCount].mTag=aTag;
  mEntries[mCount].mBankIndex=-1;
  mEntries[mCount++].mStyleIndex=-1;
}


/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
eHTMLTags nsEntryStack::Pop() {
  eHTMLTags result=eHTMLTag_unknown;
  if(0<mCount) {
    result=mEntries[--mCount].mTag;
  }
  return result;
} 
 
/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
eHTMLTags nsEntryStack::First() const {
  eHTMLTags result=eHTMLTag_unknown;
  if(0<mCount){
    result=mEntries[0].mTag;
  }
  return result;
}

/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
eHTMLTags nsEntryStack::TagAt(PRUint32 anIndex) const {
  eHTMLTags result=eHTMLTag_unknown;
  if(anIndex<mCount) {
    result=mEntries[anIndex].mTag;
  }
  return result;
}

/**
 * 
 * @update  gess 04/21/99
 */
nsTagEntry& nsEntryStack::EntryAt(PRUint32 anIndex) const {
  static nsTagEntry gSentinel;
  if(anIndex<mCount) {
    return mEntries[anIndex];
  }
  return gSentinel;
}


/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
eHTMLTags nsEntryStack::operator[](PRUint32 anIndex) const {
  eHTMLTags result=eHTMLTag_unknown;
  if(anIndex<mCount) {
    result=mEntries[anIndex].mTag;
  }
  return result;
}


/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
eHTMLTags nsEntryStack::Last() const {
  return TagAt(mCount-1);
}

/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
PRInt32 nsEntryStack::GetTopmostIndexOf(eHTMLTags aTag) const {
  int theIndex=0;
  for(theIndex=mCount-1;theIndex>=0;theIndex--){
    if(aTag==TagAt(theIndex))
      return theIndex;
  }
  return kNotFound;

}




/***************************************************************
  Now define the dtdcontext class
 ***************************************************************/


/**
 * 
 * @update	gess9/10/98
 */
nsDTDContext::nsDTDContext() : mStack(), mSkipped(0), mStyles(0) {
    
  MOZ_COUNT_CTOR(nsDTDContext);

#ifdef  NS_DEBUG
  nsCRT::zero(mTags,sizeof(mTags));
#endif
} 
 

/**
 * 
 * @update	gess9/10/98
 */
nsDTDContext::~nsDTDContext() {

  MOZ_COUNT_DTOR(nsDTDContext);

  PRInt32 theSize=mSkipped.GetSize();
  if(theSize>0) {
    CTokenDeallocator theDeallocator;
    for(PRInt32 i=0;i<theSize;i++) {
      nsDeque* theDeque=(nsDeque*)mSkipped.Pop();
      if(theDeque) {
        if(theDeque->GetSize()>0) theDeque->ForEach(theDeallocator);
        delete theDeque;
        theDeque=nsnull;
      }
    }
  }
}

/**
 * 
 * @update  gess7/9/98, harishd 04/04/99 
 */
PRInt32 nsDTDContext::GetCount(void) {
  return mStack.GetCount();
}

/**
 * 
 * @update  gess7/9/98, harishd 04/04/99
 */
void nsDTDContext::Push(eHTMLTags aTag) {
#ifdef  NS_DEBUG
  if(mStack.mCount < eMaxTags)
    mTags[mStack.mCount]=aTag;
#endif

  mStack.Push(aTag);
}

/** 
 * @update  gess7/9/98, harishd 04/04/99
 */
eHTMLTags nsDTDContext::Pop() {
#ifdef  NS_DEBUG
  if ((mStack.mCount>0) && (mStack.mCount <= eMaxTags))
    mTags[mStack.mCount-1]=eHTMLTag_unknown;
#endif

  nsEntryStack* theStyles=0;
  nsTagEntry& theEntry=mStack.EntryAt(mStack.mCount-1);
  PRInt32 theIndex=theEntry.mStyleIndex;  
  if(-1<theIndex){
    theStyles=(nsEntryStack*)mStyles.ObjectAt(theIndex);
    delete theStyles;
  }

  eHTMLTags result=mStack.Pop();
  return result;
}

/**
 * 
 * @update  gess7/9/98
 */
eHTMLTags nsDTDContext::First() const {
  return mStack.First();
}

/**
 * 
 * @update  gess7/9/98
 */
eHTMLTags nsDTDContext::TagAt(PRInt32 anIndex) const {
  return mStack.TagAt(anIndex);
}


/**
 * 
 * @update  gess7/9/98
 */
eHTMLTags nsDTDContext::operator[](PRInt32 anIndex) const {
  return mStack[anIndex];
}

/**
 * 
 * @update  gess7/9/98
 */
eHTMLTags nsDTDContext::Last() const {
  return mStack.Last();
}

/**
 * 
 * @update  gess7/9/98
 */
nsEntryStack* nsDTDContext::GetStylesAt(PRUint32 anIndex) const {
  nsEntryStack* result=0;
  if(anIndex<mStack.mCount){
    nsTagEntry& theEntry=mStack.EntryAt(anIndex);
    PRInt32 theIndex=theEntry.mStyleIndex;  
    if(-1<theIndex){
      result=(nsEntryStack*)mStyles.ObjectAt(theIndex);
    }
  }
  return result;
}

/**
 * 
 * @update  gess 04/28/99
 */
void nsDTDContext::PushStyle(eHTMLTags aTag){

  nsTagEntry& theEntry=mStack.EntryAt(mStack.mCount-1);
  //ok, now go get the right tokenbank deque...
  nsEntryStack* theStack=0;
  if(-1<theEntry.mStyleIndex)
    theStack=(nsEntryStack*)mStyles.ObjectAt(theEntry.mStyleIndex);
  if(!theStack){
    //time to make a databank for this element...
    theStack=new nsEntryStack();
    if(theStack){
      mStyles.Push(theStack);
      theEntry.mStyleIndex=mStyles.GetSize()-1;
    }
    else{
      //XXX Hack! This is very back, we've failed to get memory.
    }
  }
  if(theStack){
    theStack->Push(aTag);
  }
}

/**
 * 
 * @update  gess 04/28/99
 */
eHTMLTags nsDTDContext::PopStyle(void){
  eHTMLTags result=eHTMLTag_unknown;
  nsTagEntry& theEntry=mStack.EntryAt(mStack.mCount-1);
  //ok, now go get the right tokenbank deque...
  nsEntryStack* theStack=0;
  if(-1<theEntry.mStyleIndex)
    theStack=(nsEntryStack*)mStyles.ObjectAt(theEntry.mStyleIndex);
  if(theStack){
    result=theStack->Pop();
  }
  return result;
}


/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
void nsDTDContext::SaveToken(CToken* aToken, PRInt32 aID)
{ 
  NS_PRECONDITION(aID <= mStack.GetCount() && aID > -1,"Out of bounds");

  if(aToken) {
    nsTagEntry& theEntry=mStack.EntryAt(aID);
    //ok, now go get the right tokenbank deque...
    nsDeque* theDeque=0;
    if(-1<theEntry.mBankIndex)
      theDeque=(nsDeque*)mSkipped.ObjectAt(theEntry.mBankIndex);
    if(!theDeque){
      //time to make a databank for this element...
      theDeque=new nsDeque(0);
      if(theDeque){
        mSkipped.Push(theDeque);
        theEntry.mBankIndex=mSkipped.GetSize()-1;
      }
      else{
        //XXX Hack! This is very back, we've failed to get memory.
      }
    }
    theDeque->Push(aToken);
  }
}

/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
CToken*  nsDTDContext::RestoreTokenFrom(PRInt32 aID)
{ 
  NS_PRECONDITION(aID <= mStack.GetCount() && aID > -1,"Out of bounds");
  CToken* result=0;
  if(0<mStack.GetCount()) {
    nsTagEntry theEntry=mStack.EntryAt(aID);
    nsDeque* theDeque=(nsDeque*)mSkipped.ObjectAt(theEntry.mBankIndex);
    if(theDeque){
      result=(CToken*)theDeque->PopFront();
    }
  }
  return result;
}

/**
 * 
 * @update  harishd 04/04/99
 * @update  gess 04/21/99
 */
PRInt32  nsDTDContext::TokenCountAt(PRInt32 aID) 
{ 
  NS_PRECONDITION(aID <= mStack.GetCount(),"Out of bounds");

  nsTagEntry theEntry=mStack.EntryAt(aID);
  nsDeque* theDeque=(nsDeque*)mSkipped.ObjectAt(theEntry.mBankIndex);
  if(theDeque){
    return theDeque->GetSize();
  }

  return kNotFound;
}

/**************************************************************
  Now define the tokenrecycler class...
 **************************************************************/

/**
 * 
 * @update  gess7/25/98
 * @param 
 */
CTokenRecycler::CTokenRecycler() : nsITokenRecycler(),mEmpty("") {

  MOZ_COUNT_CTOR(CTokenRecycler);

  int i=0;
  for(i=0;i<eToken_last-1;i++) {
    mTokenCache[i]=new nsDeque(new CTokenDeallocator());
#ifdef NS_DEBUG
    mTotals[i]=0;
#endif
  }
}

/**
 * Destructor for the token factory
 * @update  gess7/25/98
 */
CTokenRecycler::~CTokenRecycler() {

  MOZ_COUNT_DTOR(CTokenRecycler);

  //begin by deleting all the known (recycled) tokens...
  //We're also deleting the cache-deques themselves.
  int i;
  for(i=0;i<eToken_last-1;i++) {
    if(0!=mTokenCache[i]) {
      delete mTokenCache[i];
      mTokenCache[i]=0;
    }
  }
}


class CTokenFinder: public nsDequeFunctor{
public:
  CTokenFinder(CToken* aToken) {mToken=aToken;}
  virtual void* operator()(void* anObject) {
    if(anObject==mToken) {
      return anObject;
    }
    return 0;
  }
  CToken* mToken;
};

/**
 * This method gets called when someone wants to recycle a token
 * @update  gess7/24/98
 * @param   aToken -- token to be recycled.
 * @return  nada
 */
void CTokenRecycler::RecycleToken(CToken* aToken) {
  if(aToken) {
    PRInt32 theType=aToken->GetTokenType();
    mTokenCache[theType-1]->Push(aToken);

#if 0
  //This should be disabled since it's only debug code.
    CTokenFinder finder(aToken);
    CToken* theMatch;
    theMatch=(CToken*)mTokenCache[theType-1]->FirstThat(finder);
#endif
  }
}


/**
 * 
 * @update	vidur 11/12/98
 * @param 
 * @return
 */
CToken* CTokenRecycler::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsString& aString) {

  CToken* result=(CToken*)mTokenCache[aType-1]->Pop();

  if(result) {
    result->Reinitialize(aTag,aString);
  }
  else {
#ifdef  NS_DEBUG
    mTotals[aType-1]++;
#endif
    switch(aType){
      case eToken_start:            result=new CStartToken(aTag); break;
      case eToken_end:              result=new CEndToken(aTag); break;
      case eToken_comment:          result=new CCommentToken(); break;
      case eToken_entity:           result=new CEntityToken(aString); break;
      case eToken_whitespace:       result=new CWhitespaceToken(); break;
      case eToken_newline:          result=new CNewlineToken(); break;
      case eToken_text:             result=new CTextToken(aString); break;
      case eToken_attribute:        result=new CAttributeToken(); break;
      case eToken_script:           result=new CScriptToken(); break;
      case eToken_style:            result=new CStyleToken(); break;
      case eToken_skippedcontent:   result=new CSkippedContentToken(aString); break;
      case eToken_instruction:      result=new CInstructionToken(); break;
      case eToken_cdatasection:     result=new CCDATASectionToken(); break;
      case eToken_error:            result=new CErrorToken(); break;
      case eToken_doctypeDecl:      result=new CDoctypeDeclToken(); break;
        default:
          break;
    }
  }
  return result;
}

/**
 * 
 * @update	vidur 11/12/98
 * @param 
 * @return
 */
CToken* CTokenRecycler::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag) {

  CToken* result=(CToken*)mTokenCache[aType-1]->Pop();

  if(result) {
    result->Reinitialize(aTag,mEmpty);
  }
  else {
#ifdef  NS_DEBUG
    mTotals[aType-1]++;
#endif
    switch(aType){
      case eToken_start:            result=new CStartToken(aTag); break;
      case eToken_end:              result=new CEndToken(aTag); break;
      case eToken_comment:          result=new CCommentToken(); break;
      case eToken_attribute:        result=new CAttributeToken(); break;
      case eToken_entity:           result=new CEntityToken(); break;
      case eToken_whitespace:       result=new CWhitespaceToken(); break;
      case eToken_newline:          result=new CNewlineToken(); break;
      case eToken_text:             result=new CTextToken(mEmpty); break;
      case eToken_script:           result=new CScriptToken(); break;
      case eToken_style:            result=new CStyleToken(); break;
      case eToken_skippedcontent:   result=new CSkippedContentToken(mEmpty); break;
      case eToken_instruction:      result=new CInstructionToken(); break;
      case eToken_cdatasection:     result=new CCDATASectionToken(); break;
      case eToken_error:            result=new CErrorToken(); break;
      case eToken_doctypeDecl:      result=new CDoctypeDeclToken(aTag); break;
        default:
          break;
    }
  }
  return result;
}

void DebugDumpContainmentRules(nsIDTD& theDTD,const char* aFilename,const char* aTitle) {
#ifdef RICKG_DEBUG

#include <fstream.h>

  const char* prefix="     ";
  fstream out(aFilename,ios::out);
  out << "==================================================" << endl;
  out << aTitle << endl;
  out << "==================================================";
  int i,j=0;
  int written;
  for(i=1;i<eHTMLTag_text;i++){
    const char* tag=nsHTMLTags::GetStringValue((eHTMLTags)i);
    out << endl << endl << "Tag: <" << tag << ">" << endl;
    out << prefix;
    written=0;
    if(theDTD.IsContainer(i)) {
      for(j=1;j<eHTMLTag_text;j++){
        if(15==written) {
          out << endl << prefix;
          written=0;
        }
        if(theDTD.CanContain(i,j)){
          tag=nsHTMLTags::GetStringValue((eHTMLTags)j);
          if(tag) {
            out<< tag << ", ";
            written++;
          }
        }
      }//for
    }
    else out<<"(not container)" << endl;
  }
#endif
}


/*************************************************************************
 *  The table lookup technique was adapted from the algorithm described  *
 *  by Avram Perez, Byte-wise CRC Calculations, IEEE Micro 3, 40 (1983). *
 *************************************************************************/

#define POLYNOMIAL 0x04c11db7L

static PRBool crc_table_initialized;
static PRUint32 crc_table[256];

static void gen_crc_table() {
 /* generate the table of CRC remainders for all possible bytes */
  int i, j;  
  PRUint32 crc_accum;
  for ( i = 0;  i < 256;  i++ ) { 
    crc_accum = ( (unsigned long) i << 24 );
    for ( j = 0;  j < 8;  j++ ) { 
      if ( crc_accum & 0x80000000L )
        crc_accum = ( crc_accum << 1 ) ^ POLYNOMIAL;
      else crc_accum = ( crc_accum << 1 ); 
    }
    crc_table[i] = crc_accum; 
  }
  return; 
}

PRUint32 AccumulateCRC(PRUint32 crc_accum, char *data_blk_ptr, int data_blk_size)  {
  if (!crc_table_initialized) {
    gen_crc_table();
    crc_table_initialized = PR_TRUE;
  }

 /* update the CRC on the data block one byte at a time */
  int i, j;
  for ( j = 0;  j < data_blk_size;  j++ ) { 
    i = ( (int) ( crc_accum >> 24) ^ *data_blk_ptr++ ) & 0xff;
    crc_accum = ( crc_accum << 8 ) ^ crc_table[i]; 
  }
  return crc_accum; 
}

/******************************************************************************
  This class is used to store ref's to tag observers during the parse phase.
  Note that for simplicity, this is a singleton that is constructed in the
  CNavDTD and shared for the duration of the application session. Later on it
  might be nice to use a more dynamic approach that would permit observers to
  come and go on a document basis.
 ******************************************************************************/

CObserverService::CObserverService() {

  MOZ_COUNT_CTOR(CObserverService);

  nsCRT::zero(mObservers,sizeof(mObservers));

  nsAutoString theHTMLTopic("htmlparser");
  RegisterObservers(theHTMLTopic);

  nsAutoString theXMLTopic("xmlparser");
  RegisterObservers(theXMLTopic);

}

CObserverService::~CObserverService() {

  MOZ_COUNT_DTOR(CObserverService);

  UnregisterObservers();
}

/**************************************************************
  Define the nsIElementObserver release class...
 **************************************************************/
class nsObserverReleaser: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    nsIElementObserver* theObserver= (nsIElementObserver*)anObject;
    NS_RELEASE(theObserver);
    return 0;
  }
};

/**
 * Release observers and empty the observer array.
 * 
 * @update harishd 08/29/99
 * @param  
 * @return 
 */

void CObserverService::UnregisterObservers() {
  int theIndex=0;
  nsObserverReleaser theReleaser;
  for(theIndex=0;theIndex<=NS_HTML_TAG_MAX;theIndex++){
    if(mObservers[theIndex]){
      mObservers[theIndex]->ForEach(theReleaser);
      delete mObservers[theIndex];
    }
  }
}

/**
 * This method will maintain lists of observers registered for specific tags.
 * 
 * @update harishd 08/29/99
 * @param  aTopic  -  The topic under which observers register for.
 * @return if SUCCESS return NS_OK else return ERROR code.
 */

void CObserverService::RegisterObservers(nsString& aTopic) {
  nsresult result = NS_OK;
  nsIObserverService* theObserverService = nsnull;
  result = nsServiceManager::GetService(NS_OBSERVERSERVICE_PROGID, nsIObserverService::GetIID(),
                                      (nsISupports**) &theObserverService, nsnull);
  if(result == NS_OK){
    nsIEnumerator* theEnum = nsnull;
    result = theObserverService->EnumerateObserverList(aTopic.GetUnicode(), &theEnum);
    nsServiceManager::ReleaseService(NS_OBSERVERSERVICE_PROGID, theObserverService);
    if(result == NS_OK) {
      nsIElementObserver *theElementObserver = nsnull;
      nsISupports *inst = nsnull;
      
      for (theEnum->First(); theEnum->IsDone() != NS_OK; theEnum->Next()) {
        result = theEnum->CurrentItem(&inst);
        if (NS_SUCCEEDED(result)) {
          result = inst->QueryInterface(nsIElementObserver::GetIID(), (void**)&theElementObserver);
          NS_RELEASE(inst);
        }
        if (result == NS_OK) {
          const char* theTagStr = nsnull;
          PRUint32 theTagIndex = 0;
          theTagStr = theElementObserver->GetTagNameAt(theTagIndex);
          while (theTagStr != nsnull) {
            eHTMLTags theTag = nsHTMLTags::LookupTag(nsCAutoString(theTagStr));
            if((eHTMLTag_userdefined!=theTag) && (theTag <= NS_HTML_TAG_MAX)){
              if(mObservers[theTag] == nsnull) {
                 mObservers[theTag] = new nsDeque(0);
              }
              mObservers[theTag]->Push(theElementObserver);
            }
            theTagIndex++;
            theTagStr = theElementObserver->GetTagNameAt(theTagIndex);
          }
        }
      }
    }
    NS_IF_RELEASE(theEnum);
  }
}

/**
 * This method will notify observers registered for specific tags.
 * 
 * @update harishd 08/29/99
 * @param  aTag            -  The tag for which observers could be waiting for.
 * @param  aNode           -  
 * @param  aUniqueID       -  The document ID.
 * @param  aDTD            -  The current DTD.
 * @param  aCharsetValue   -
 * @param  aCharsetSource  -
 * @return if SUCCESS return NS_OK else return ERROR code.
 */
nsresult CObserverService::Notify(eHTMLTags aTag,nsIParserNode& aNode,PRUint32 aUniqueID, const char* aCommand,
                                  nsAutoString& aCharsetValue,nsCharsetSource& aCharsetSource) {
  nsresult  result=NS_OK;
  nsDeque*  theDeque=GetObserversForTag(aTag);
  if(theDeque){ 
    PRInt32 theAttrCount =aNode.GetAttributeCount(); 
    PRUint32 theDequeSize=theDeque->GetSize(); 
    if(0<theDequeSize){
      PRInt32 index = 0; 
      const PRUnichar* theKeys[50]  = {0,0,0,0,0}; // XXX -  should be dynamic
      const PRUnichar* theValues[50]= {0,0,0,0,0}; // XXX -  should be dynamic
      for(index=0; index<theAttrCount && index < 50; index++) {
        theKeys[index]   = aNode.GetKeyAt(index).GetUnicode(); 
        theValues[index] = aNode.GetValueAt(index).GetUnicode(); 
      } 
      nsAutoString theCharsetKey("charset"); 
      nsAutoString theSourceKey("charsetSource"); 
      nsAutoString intValue;
      // Add pseudo attribute in the end
      if(index < 50) {
        theKeys[index]=theCharsetKey.GetUnicode(); 
        theValues[index] = aCharsetValue.GetUnicode();
        index++;
      }
      if(index < 50) {
        theKeys[index]=theSourceKey.GetUnicode(); 
        PRInt32 sourceInt = aCharsetSource;
        intValue.Append(sourceInt,10);
        theValues[index] = intValue.GetUnicode();
	  	  index++;
      }
      if(index < 50) {
        nsAutoString theDTDKey("X_COMMAND");
        nsAutoString theDTDValue(aCommand);
        theKeys[index]=theDTDKey.GetUnicode();
        theValues[index]=theDTDValue.GetUnicode();
        index++;
      }
      nsAutoString theTagStr(nsHTMLTags::GetStringValue(aTag));
      nsObserverNotifier theNotifier(theTagStr.GetUnicode(),aUniqueID,index,theKeys,theValues);
      theDeque->FirstThat(theNotifier); 
      result=theNotifier.mResult; 
     }//if 
  } 
  return result;
}

/**
 * This method will look for the list of observers registered for 
 * a specific tag.
 * 
 * @update harishd 08/29/99
 * @param aTag - The tag for which observers could be waiting for.
 * @return if FOUND return "observer list" else return nsnull;
 *
 */

nsDeque* CObserverService::GetObserversForTag(eHTMLTags aTag) {
  if(aTag <= NS_HTML_TAG_MAX)
      return mObservers[aTag];
  return nsnull;
}
