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

#include "msgCore.h"    // precompiled header...
#include "nntpCore.h"
#include "nsMsgNewsCID.h"
#include "nsINntpUrl.h"
#include "nsNNTPProtocol.h"
#include "nsNNTPNewsgroupPost.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIdentity.h"
#include "nsString.h"
#include "nsNewsUtils.h"
#include "nsNewsDatabase.h"
#include "nsMsgDBCID.h"
#include "nsNNTPNewsgroup.h"
#include "nsMsgBaseCID.h"
#include "nsMsgNewsCID.h"
#include "nsIMessage.h"
#include "nsINetSupportDialogService.h"
#include "nsIPref.h"
#include "nsCRT.h"  // for nsCRT::strtok
#include "nsNntpService.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsIFileLocator.h"
#include "nsFileLocations.h"

#undef GetPort  // XXX Windows!
#undef SetPort  // XXX Windows!

#define PREF_NETWORK_HOSTS_NNTP_SERVER	"network.hosts.nntp_server"
#define PREF_MAIL_ROOT_NNTP 	"mail.root.nntp"

static NS_DEFINE_CID(kCNntpUrlCID, NS_NNTPURL_CID);
static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
static NS_DEFINE_CID(kCNNTPNewsgroupCID, NS_NNTPNEWSGROUP_CID);
static NS_DEFINE_CID(kCNNTPNewsgroupPostCID, NS_NNTPNEWSGROUPPOST_CID);
static NS_DEFINE_CID(kCNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);   
static NS_DEFINE_CID(kCPrefServiceCID, NS_PREF_CID); 
static NS_DEFINE_IID(kIFileLocatorIID,      NS_IFILELOCATOR_IID);
static NS_DEFINE_CID(kFileLocatorCID,       NS_FILELOCATOR_CID);
                     
nsNntpService::nsNntpService()
{
    NS_INIT_REFCNT();
}

nsNntpService::~nsNntpService()
{
  // do nothing
}

NS_IMPL_THREADSAFE_ADDREF(nsNntpService);
NS_IMPL_THREADSAFE_RELEASE(nsNntpService);

NS_IMPL_QUERY_INTERFACE4(nsNntpService,
                         nsINntpService,
                         nsIMsgMessageService,
                         nsIProtocolHandler,
                         nsIMsgProtocolInfo) 

////////////////////////////////////////////////////////////////////////////////////////
// nsIMsgMessageService support
////////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP nsNntpService::SaveMessageToDisk(const char *aMessageURI, nsIFileSpec *aFile, PRBool aAddDummyEnvelope, nsIUrlListener *aUrlListener, nsIURI **aURL)
{
    nsresult rv = NS_OK;
    if (!aMessageURI) 
        return NS_ERROR_NULL_POINTER;

#ifdef DEBUG_NEWS
    printf("nsNntpService::SaveMessageToDisk(%s,...)\n",aMessageURI);
#endif

    nsCAutoString uri(aMessageURI);
    nsCAutoString newsgroupName;
    nsMsgKey key = nsMsgKey_None;
    
    if (PL_strncmp(aMessageURI, kNewsMessageRootURI, kNewsMessageRootURILen) == 0)
	    rv = ConvertNewsMessageURI2NewsURI(aMessageURI, uri, newsgroupName, &key);
    else
        return NS_ERROR_UNEXPECTED;

    // now create a url with this uri spec
    nsCOMPtr<nsIURI> myuri;

    rv = ConstructNntpUrl(uri, newsgroupName, key, aUrlListener, getter_AddRefs(myuri));
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(myuri);
        nsCOMPtr<nsIMsgMessageUrl> msgUrl = do_QueryInterface(myuri);
        nntpUrl->SetNewsAction(nsINntpUrl::ActionSaveMessageToDisk);
        if (msgUrl)
        {
		    msgUrl->SetMessageFile(aFile);
            msgUrl->SetAddDummyEnvelope(aAddDummyEnvelope);
        }   
    
        RunNewsUrl(myuri, nsnull);
    }

    if (aURL)
    {
	    *aURL = myuri;
	    NS_IF_ADDREF(*aURL);
    }

  return rv;
}

nsresult nsNntpService::DisplayMessage(const char* aMessageURI, nsISupports * aDisplayConsumer, nsIUrlListener * aUrlListener, nsIURI ** aURL)
{
  nsresult rv = NS_OK;
  
  if (!aMessageURI) {
    return NS_ERROR_NULL_POINTER;
  }

#ifdef DEBUG_NEWS
  printf("nsNntpService::DisplayMessage(%s,...)\n",aMessageURI);
#endif

  nsCAutoString uri(aMessageURI);
  nsCAutoString newsgroupName;
  nsMsgKey key = nsMsgKey_None;
    
  if (PL_strncmp(aMessageURI, kNewsMessageRootURI, kNewsMessageRootURILen) == 0)
	rv = ConvertNewsMessageURI2NewsURI(aMessageURI, uri, newsgroupName, &key);
  else
    return NS_ERROR_UNEXPECTED;

  // now create a url with this uri spec
  nsCOMPtr<nsIURI> myuri;

  rv = ConstructNntpUrl(uri, newsgroupName, key, aUrlListener, getter_AddRefs(myuri));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(myuri);
    nntpUrl->SetNewsAction(nsINntpUrl::ActionDisplayArticle);

    // now is where our behavior differs....if the consumer is the webshell then we want to 
    // run the url in the webshell in order to display it. If it isn't a webshell then just
    // run the news url like we would any other news url. 
	  nsCOMPtr<nsIWebShell> webshell = do_QueryInterface(aDisplayConsumer, &rv);
    if (NS_SUCCEEDED(rv) && webshell)
	    rv = webshell->LoadURI(myuri, "view", nsnull, PR_TRUE);
    else
      rv = RunNewsUrl(myuri, aDisplayConsumer);
  }

  if (aURL)
  {
	  *aURL = myuri;
	  NS_IF_ADDREF(*aURL);
  }

  return rv;
}

NS_IMETHODIMP nsNntpService::GetUrlForUri(const char *aMessageURI, nsIURI **aURL) 
{
  nsresult rv = NS_OK;
  nsCAutoString uri(aMessageURI);
  nsCAutoString newsgroupName;
  nsMsgKey key = nsMsgKey_None;
    
  if (PL_strncmp(aMessageURI, kNewsMessageRootURI, kNewsMessageRootURILen) == 0)
  {
	  rv = ConvertNewsMessageURI2NewsURI(aMessageURI, uri, newsgroupName, &key);
    if (NS_SUCCEEDED(rv))
      rv = ConstructNntpUrl(uri, newsgroupName, key, nsnull, aURL);
  }
  else 
    rv = NS_ERROR_UNEXPECTED;

  return rv;

}

nsresult nsNntpService::ConvertNewsMessageURI2NewsURI(const char *messageURI, nsCString &newsURI, nsCString &newsgroupName, nsMsgKey *key)
{
  nsCAutoString hostname;
  nsCAutoString messageUriWithoutKey;
  nsresult rv = NS_OK;

  // messageURI is of the form:  "news_message://news.mcom.com/mcom.linux#1"
  // if successful, we should get
  // messageUriWithoutKey = "news_message://news.mcom.com/mcom.linux"
  // key = 1
  rv = nsParseNewsMessageURI(messageURI, messageUriWithoutKey, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // turn news_message://news.mcom.com/mcom.linux -> news.mcom.com/mcom.linux
  // stick "news.mcom.com/mcom.linux" in hostname.
  messageUriWithoutKey.Right(hostname, messageUriWithoutKey.Length() - kNewsMessageRootURILen - 1);

  // take news.mcom.com/mcom.linux (in hostname) and put
  // "mcom.linux" into newsgroupName and truncate to leave
  // "news.mcom.com" in hostname
  PRInt32 hostEnd = hostname.FindChar('/');
  if (hostEnd > 0) {
    hostname.Right(newsgroupName, hostname.Length() - hostEnd - 1);
    hostname.Truncate(hostEnd);
  }
  else {
    // error!
    // we didn't find a "/" in something we thought looked like this:
    // news.mcom.com/mcom.linux
    return NS_ERROR_FAILURE;
  }

#ifdef DEBUG_NEWS
  printf("ConvertNewsMessageURI2NewsURI(%s,??) -> %s %u\n", messageURI, newsgroupName.GetBuffer(), *key);
#endif

  nsFileSpec pathResult;

  rv = nsNewsURI2Path(kNewsMessageRootURI, messageUriWithoutKey, pathResult);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIMsgDatabase> newsDBFactory;
  nsCOMPtr<nsIMsgDatabase> newsDB;
  
  rv = nsComponentManager::CreateInstance(kCNewsDB, nsnull, nsIMsgDatabase::GetIID(), getter_AddRefs(newsDBFactory));
  if (NS_FAILED(rv) || (!newsDBFactory)) {
    return rv;
  }

  nsCOMPtr <nsIFileSpec> dbFileSpec;
  NS_NewFileSpecWithSpec(pathResult, getter_AddRefs(dbFileSpec));
  rv = newsDBFactory->Open(dbFileSpec, PR_TRUE, PR_FALSE, getter_AddRefs(newsDB));
    
  if (NS_FAILED(rv) || (!newsDB)) {
    return rv;
  }

  nsCOMPtr<nsIMsgDBHdr> msgHdr;
  
  rv = newsDB->GetMsgHdrForKey((nsMsgKey) *key, getter_AddRefs(msgHdr));
  if (NS_FAILED(rv) || (!msgHdr)) {
    return rv;
  }

  nsCString messageId;
  rv = msgHdr->GetMessageId(&messageId);

#ifdef DEBUG_NEWS
  PRUint32 bytes;
  PRUint32 lines;
  rv = msgHdr->GetMessageSize(&bytes);
  rv = msgHdr->GetLineCount(&lines);

  printf("bytes = %u\n",bytes);
  printf("lines = %u\n",lines);
#endif

  if (NS_FAILED(rv)) {
    return rv;
  }

  newsURI = kNewsRootURI;
  newsURI += "/";
  newsURI += hostname;
  newsURI += "/";
  newsURI += messageId;

#ifdef DEBUG_NEWS
  printf("newsURI = %s\n", (const char *)nsAutoCString(newsURI));
#endif

  return NS_OK;
}


nsresult nsNntpService::CopyMessage(const char * aSrcMailboxURI, nsIStreamListener * aMailboxCopyHandler, PRBool moveMessage,
						   nsIUrlListener * aUrlListener, nsIURI **aURL)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsISupports> streamSupport;
    if (!aSrcMailboxURI || !aMailboxCopyHandler) return rv;
    streamSupport = do_QueryInterface(aMailboxCopyHandler, &rv);
    if (NS_SUCCEEDED(rv))
        rv = DisplayMessage(aSrcMailboxURI, streamSupport, aUrlListener, aURL);
	return rv;
}

nsresult nsNntpService::FindHostFromGroup(nsCString &host, nsCString &groupName)
{
  nsresult rv = NS_OK;
  // host always comes in as ""
  NS_ASSERTION(host.IsEmpty(), "host is not empty");
  if (!host.IsEmpty()) return NS_ERROR_FAILURE;
  
  if (0) {
    // groupName is "foobar"
    // todo:  look for "foobar" in the newsrc files of all the hosts,
    // starting with the default nntp host.
    //
    // for now, leave host blank so we'll just use the default nntp host.
  }
  
  if (host.IsEmpty()) {
    NS_WITH_SERVICE(nsIPref, prefs, kCPrefServiceCID, &rv);
    if (NS_FAILED(rv) || (!prefs)) {
      return rv;
    } 
    
    char *default_nntp_server = nsnull; 
    // if we get here, we know prefs is not null
    rv = prefs->CopyCharPref(PREF_NETWORK_HOSTS_NNTP_SERVER, &default_nntp_server);
    if (NS_FAILED(rv) || (!default_nntp_server)) {
      // if all else fails, use "news" as the default_nntp_server
      default_nntp_server = PR_smprintf("news");
    }
    host = default_nntp_server;
    PR_FREEIF(default_nntp_server);
  }
  
  if (host.IsEmpty())
    return NS_ERROR_FAILURE;
  else 
    return NS_OK;
}

nsresult nsNntpService::DetermineHostForPosting(nsCString &host, const char *newsgroupsNames)
{
  nsresult rv = NS_OK;
  
  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;

#ifdef DEBUG_NEWS
  printf("newsgroupsNames == %s\n",newsgroupsNames);
#endif
  
  // newsgroupsNames can be a comma seperated list of these:
  // news://host/group
  // news://group
  // host/group
  // group
  //
  // we are not going to allow the user to cross post to multiple hosts.
  // so as soon as we determine the host from newsgroupsNames, we can stop.
  //
  // 1) explict (news://host/group or host/group)
  // 2) context (if you reply to a message when reading it on host x, reply to that host.
  // 3) no context look up news://group or group in the various newsrc files to determine host, with PREF_NETWORK_HOSTS_NNTP_SERVER being the first newrc file searched each time
  // 4) use the default nntp server

  //nsCRT::strtok is going destroy what we pass to it, so we need to make a copy of newsgroupsNames.
  char *list = PL_strdup(newsgroupsNames);
  char *token = nsnull;
  char *rest = list;
  nsCAutoString str;
  
  token = nsCRT::strtok(rest, ",", &rest);
  while (token && *token) {
    str = token;
    str.StripWhitespace();

    if (!str.IsEmpty()) {
#ifdef DEBUG_NEWS
      printf("value = %s\n", str.GetBuffer());
#endif
      nsCAutoString theRest;
      nsCAutoString currentHost;
      
      // does str start with "news:/"?
      if (str.Find(kNewsRootURI) == 0) {
        // we have news://group or news://host/group
        // set theRest to what's after news://
        str.Right(theRest, str.Length() - kNewsRootURILen /* for news:/ */ - 1 /* for the slash */);
      }
      else if (str.Find(":/") != -1) {
#ifdef DEBUG_NEWS
	printf("we have x:/y where x != news. this is bad, return failure\n");
#endif
        PR_FREEIF(list);
        return NS_ERROR_FAILURE;
      }
      else {
        theRest = str;
      }
      
#ifdef DEBUG_NEWS
      printf("theRest == %s\n",theRest.GetBuffer());
#endif
      
      // theRest is "group" or "host/group"
      PRInt32 slashpos = theRest.FindChar('/');
      if (slashpos > 0 ) {
        // theRest is "host/group"
        theRest.Left(currentHost, slashpos);
#ifdef DEBUG_NEWS
        printf("currentHost == %s\n", currentHost.GetBuffer());
#endif
      }
      else {
        // theRest is "group"
        rv = FindHostFromGroup(currentHost, str);
        PR_FREEIF(list);
        if (NS_FAILED(rv)) return rv;
      }

      if (host.IsEmpty()) {
        host = currentHost;

        //we have our host, we're done.
        //break;
      }
      else {
        if (host != currentHost) {
          printf("no cross posting to multiple hosts!\n");
          PR_FREEIF(list);
          return NS_ERROR_FAILURE;
        }
      }
      
      str = "";
      currentHost = "";
    }
#ifdef DEBUG_NEWS
    else {
        printf("nothing between two commas. ignore and keep going...\n");
    }
#endif
    token = nsCRT::strtok(rest, ",", &rest);
  }    
  PR_FREEIF(list);
  
  if (!host.IsEmpty())
    return NS_OK;
  else
    return NS_ERROR_FAILURE;
}
////////////////////////////////////////////////////////////////////////////////////////
// nsINntpService support
////////////////////////////////////////////////////////////////////////////////////////
nsresult nsNntpService::ConvertNewsgroupsString(const char *newsgroupsNames, char **_retval)
{
  nsresult rv = NS_OK;
  
  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;

#ifdef DEBUG_NEWS
  printf("newsgroupsNames == %s\n",newsgroupsNames);
#endif
  
  // newsgroupsNames can be a comma seperated list of these:
  // news://host/group
  // news://group
  // host/group
  // group
  //
  // we are not going to allow the user to cross post to multiple hosts.
  // if we detect that, we stop and return error.

  // nsCRT::strtok is going destroy what we pass to it, so we need to make a copy of newsgroupsNames.
  char *list = PL_strdup(newsgroupsNames);
  char *token = nsnull;
  char *rest = list;
  nsCAutoString host;
  nsCAutoString str;
  nsCAutoString retvalStr;
    
  token = nsCRT::strtok(rest, ",", &rest);
  while (token && *token) {
    str = token;
    str.StripWhitespace();

    if (!str.IsEmpty()) {
#ifdef DEBUG_NEWS
      printf("value = %s\n", str.GetBuffer());
#endif
      nsCAutoString currentHost;
      nsCAutoString theRest;

      // does str start with "news:/"?
      if (str.Find(kNewsRootURI) == 0) {
        // we have news://group or news://host/group
        // set theRest to what's after news://
        str.Right(theRest, str.Length() - kNewsRootURILen /* for news:/ */ - 1 /* for the slash */);
      }
      else if (str.Find(":/") != -1) {
#ifdef DEBUG_NEWS
	printf("we have x:/y where x != news. this is bad, return failure\n");
#endif
        PR_FREEIF(list);
        return NS_ERROR_FAILURE;
      }
      else {
        theRest = str;
      }
      
#ifdef DEBUG_NEWS
      printf("theRest == %s\n",theRest.GetBuffer());
#endif
      
      // theRest is "group" or "host/group"
      PRInt32 slashpos = theRest.FindChar('/');
      if (slashpos > 0 ) {
        nsCAutoString currentGroup;
        
        // theRest is "host/group"
        theRest.Left(currentHost, slashpos);
        
#ifdef DEBUG_NEWS
        printf("currentHost == %s\n", currentHost.GetBuffer());
#endif
        // from "host/group", put "group" into currentGroup;
        theRest.Right(currentGroup, theRest.Length() - currentHost.Length() - 1);

        NS_ASSERTION(!currentGroup.IsEmpty(), "currentGroup is empty");
        if (currentGroup.IsEmpty()) {
          PR_FREEIF(list);
          return NS_ERROR_FAILURE;
        }
        
        // build up the retvalStr;
        if (!retvalStr.IsEmpty()) {
          retvalStr += ",";
        }
        retvalStr += currentGroup;
      }
      else {
        // theRest is "group"
        rv = FindHostFromGroup(currentHost, str);
        if (NS_FAILED(rv)) {
            PR_FREEIF(list);
            return rv;
        }

        // build up the retvalStr;
        if (!retvalStr.IsEmpty()) {
          retvalStr += ",";
        }
        retvalStr += str;
      }

      if (currentHost.IsEmpty()) {
#ifdef DEBUG_sspitzer
        printf("empty current host!\n");
#endif
        PR_FREEIF(list);
        return NS_ERROR_FAILURE;
      }
      
      if (host.IsEmpty()) {
#ifdef DEBUG_sspitzer
        printf("got a host, set it\n");
#endif
        host = currentHost;
      }
      else {
        if (host != currentHost) {
#ifdef DEBUG_sspitzer
          printf("no cross posting to multiple hosts!\n");
#endif
          PR_FREEIF(list);
          return NS_ERROR_FAILURE;
        }
      }

      str = "";
      currentHost = "";
    }
#ifdef DEBUG_NEWS
    else {
        printf("nothing between two commas. ignore and keep going...\n");
    }
#endif
    token = nsCRT::strtok(rest, ",", &rest);
  }
  PR_FREEIF(list);
  
  // caller will free with PR_FREEIF()
  *_retval = PL_strdup(retvalStr.GetBuffer());
  if (!*_retval) return NS_ERROR_OUT_OF_MEMORY;
  
#ifdef DEBUG_NEWS
  printf("Newsgroups header = %s\n", *_retval);
#endif

  return NS_OK;
}

nsresult nsNntpService::PostMessage(nsIFileSpec *fileToPost, const char *newsgroupsNames, nsIUrlListener * aUrlListener, nsIURI **_retval)
{
#ifdef DEBUG_NEWS
  printf("nsNntpService::PostMessage(??,%s,??,??)\n",newsgroupsNames);
#endif
  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;
    
  NS_LOCK_INSTANCE();
  
  nsCOMPtr <nsINntpUrl> nntpUrl;
  nsresult rv = NS_OK;
  
  rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, nsINntpUrl::GetIID(), getter_AddRefs(nntpUrl));
  if (NS_FAILED(rv) || !nntpUrl) return rv;

  nntpUrl->SetNewsAction(nsINntpUrl::ActionPostArticle);

  nsCAutoString host;
  rv = DetermineHostForPosting(host, newsgroupsNames);
  
  if (NS_FAILED(rv) || (host.IsEmpty())) return rv;

#ifdef DEBUG_NEWS
  printf("post to this host: %s\n",host.GetBuffer());
#endif

  char *urlstr = PR_smprintf("%s/%s",kNewsRootURI,host.GetBuffer());
  nsCOMPtr<nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(nntpUrl);
  if (mailnewsurl) {
    mailnewsurl->SetSpec(urlstr);
    mailnewsurl->SetPort(NEWS_PORT);
  }
  else {
    return NS_ERROR_FAILURE;
  }
  
  PR_FREEIF(urlstr);
  
  if (aUrlListener) // register listener if there is one...
    mailnewsurl->RegisterListener(aUrlListener);
  
  // almost there...now create a nntp protocol instance to run the url in...
  nsNNTPProtocol *nntpProtocol = nsnull;

  nntpProtocol = new nsNNTPProtocol(mailnewsurl);
  if (!nntpProtocol) return NS_ERROR_OUT_OF_MEMORY;;
  
  rv = nntpProtocol->Initialize();
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr <nsINNTPNewsgroupPost> post;
  rv = nsComponentManager::CreateInstance(kCNNTPNewsgroupPostCID, nsnull, nsINNTPNewsgroupPost::GetIID(), getter_AddRefs(post));
  if (NS_FAILED(rv) || !post) return rv;

  rv = post->SetPostMessageFile(fileToPost);
  if (NS_FAILED(rv)) return rv;
  
  rv = nntpUrl->SetMessageToPost(post);
  if (NS_FAILED(rv)) return rv;
            
  rv = nntpProtocol->LoadUrl(mailnewsurl, /* aConsumer */ nsnull);
		
  if (_retval)
	  nntpUrl->QueryInterface(nsIURI::GetIID(), (void **) _retval);
    
  NS_UNLOCK_INSTANCE();

  return rv;
}

nsresult nsNntpService::ConstructNntpUrl(const char * urlString, const char * newsgroupName, nsMsgKey key, nsIUrlListener *aUrlListener,  nsIURI ** aUrl)
{
  nsCOMPtr <nsINntpUrl> nntpUrl;
  nsresult rv = NS_OK;

  rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, nsINntpUrl::GetIID(), getter_AddRefs(nntpUrl));
  if (NS_FAILED(rv) || !nntpUrl) return rv;
  
  nsCOMPtr <nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(nntpUrl);
  // don't worry this cast is really okay...there'a bug in XPIDL compiler that is preventing
  // a "cont char *" in paramemter for uri SetSpec...
  mailnewsurl->SetSpec((char *) urlString);
  mailnewsurl->SetPort(NEWS_PORT);

  if (newsgroupName != "") {
    nsCOMPtr <nsINNTPNewsgroup> newsgroup;
    rv = nsComponentManager::CreateInstance(kCNNTPNewsgroupCID, nsnull, nsINNTPNewsgroup::GetIID(), getter_AddRefs(newsgroup));
    if (NS_FAILED(rv) || !newsgroup) return rv;                       
       
    rv = newsgroup->Initialize(newsgroupName, nsnull /* set */, PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    
    rv = nntpUrl->SetNewsgroup(newsgroup);
    if (NS_FAILED(rv)) return rv;
    
    // if we are running a news url to display a message, these
    // will be used later, to mark the message as read after we finish loading
    rv = nntpUrl->SetMessageKey(key);
    if (NS_FAILED(rv)) return rv;
        
    rv = nntpUrl->SetNewsgroupName((char *) newsgroupName);
    if (NS_FAILED(rv)) return rv;
  }
  
  if (aUrlListener) // register listener if there is one...
    mailnewsurl->RegisterListener(aUrlListener);

  (*aUrl) = mailnewsurl;
  NS_IF_ADDREF(*aUrl);
  return rv;

}

nsresult 
nsNntpService::RunNewsUrl(nsIURI * aUri, nsISupports * aConsumer)
{
  // almost there...now create a nntp protocol instance to run the url in...
  nsNNTPProtocol *nntpProtocol = nsnull;

  nntpProtocol = new nsNNTPProtocol(aUri);
  if (!nntpProtocol) return NS_ERROR_OUT_OF_MEMORY;
  
  nsresult rv = nntpProtocol->Initialize();
  if (NS_FAILED(rv)) return rv;
  
  rv = nntpProtocol->LoadUrl(aUri, aConsumer);
  return rv;
}

NS_IMETHODIMP nsNntpService::GetNewNews(nsINntpIncomingServer *nntpServer, const char *uri, nsIUrlListener * aUrlListener, nsIURI **_retval)
{
  if (!uri) {
	return NS_ERROR_NULL_POINTER;
  }

#ifdef DEBUG_NEWS
  printf("nsNntpService::GetNewNews(%s)\n", uri);
#endif
  
  NS_LOCK_INSTANCE();
  nsresult rv = NS_OK;
  nsXPIDLCString nntpHostName;
  
  nsCOMPtr<nsIMsgIncomingServer> server;
  server = do_QueryInterface(nntpServer);
  
  // convert normal host to nntp host.
  // XXX - this doesn't handle QI failing very well
  if (server) {
    // load up required server information
    server->GetHostName(getter_Copies(nntpHostName));
  }
#ifdef DEBUG_NEWS
  else {
	  printf("server == nsnull\n");
  }
#endif
  
#ifdef DEBUG_NEWS
  if (nntpHostName) {
    printf("get news from news://%s\n", (const char *) nntpHostName);
  }
  else {
    printf("nntpHostName is null\n");
  }
#endif

  nsCAutoString uriStr = uri;
  nsCAutoString newsgroupName;
  
  NS_ASSERTION((uriStr.Find(kNewsRootURI) == 0), "uriStr didn't start with news:/");
  if (uriStr.Find(kNewsRootURI) == 0) {
    // uriStr looks like this:  
    // "news://news.mcom.com/mcom.linux"
    // or this:
    // "news://sspitzer@news.mcom.com/mcom.linux"

    PRInt32 atPos = uriStr.FindChar('@');
    if (atPos == -1) {
	    uriStr.Right(newsgroupName, uriStr.Length() - kNewsRootURILen /* for news:/ */ - 1 /* for the slash */ - PL_strlen(nntpHostName) /* for the hostname */ -1 /* for the next slash */);
    }
    else {
	    uriStr.Right(newsgroupName, uriStr.Length() - atPos /* for "news://<username>" */ -1 /* for the @ */ - PL_strlen(nntpHostName) /* for the hostname */ -1 /* for the next slash */);
    }
    
	nsCOMPtr<nsIURI> aUrl;
	rv = ConstructNntpUrl(uriStr, newsgroupName, nsMsgKey_None, aUrlListener,  getter_AddRefs(aUrl));
	if (NS_FAILED(rv)) return rv;
	nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(aUrl);
	if (nntpUrl)
		nntpUrl->SetNewsAction(nsINntpUrl::ActionGetNewNews);
	nsCOMPtr<nsIMsgMailNewsUrl> mailNewsUrl = do_QueryInterface(aUrl);
	if (mailNewsUrl)
		mailNewsUrl->SetUpdatingFolder(PR_TRUE);

    rv = RunNewsUrl(aUrl, nsnull);  
	
	if (_retval)
	{
		*_retval = aUrl;
		NS_IF_ADDREF(*_retval);
	}
  }
  else 
    rv = NS_ERROR_FAILURE;
  
      
  NS_UNLOCK_INSTANCE();
  return rv;
}

NS_IMETHODIMP nsNntpService::CancelMessages(const char *hostname, const char *newsgroupname, nsISupportsArray *messages, nsISupports * aConsumer, nsIUrlListener * aUrlListener, nsIURI ** aURL)
{
  nsresult rv = NS_OK;
  PRUint32 count = 0;

  if (!hostname) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(hostname) == 0) return NS_ERROR_FAILURE;

  NS_WITH_SERVICE(nsIPrompt, dialog, kCNetSupportDialogCID, &rv);
  if (NS_FAILED(rv)) return rv;
    
  if (!messages) {
    nsAutoString alertText("No articles are selected.");
    if (dialog)
      rv = dialog->Alert(alertText.GetUnicode());
    
    return NS_ERROR_NULL_POINTER;
  }

  rv = messages->Count(&count);
  if (NS_FAILED(rv)) {
#ifdef DEBUG_NEWS
    printf("Count failed\n");
#endif
    return rv;
  }
  
  if (count != 1) {
    nsAutoString alertText("You can only cancel one article at a time.");
    if (dialog)
      rv = dialog->Alert(alertText.GetUnicode());
    return NS_ERROR_FAILURE;
  }
  
  nsMsgKey key;
  nsCString messageId;
  
  nsCOMPtr<nsISupports> msgSupports = getter_AddRefs(messages->ElementAt(0));
  nsCOMPtr<nsIMessage> message(do_QueryInterface(msgSupports));
  if (message) {
    rv = message->GetMessageKey(&key);
    if (NS_FAILED(rv)) return rv;
    rv = message->GetMessageId(&messageId);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    return NS_ERROR_FAILURE;
  }
  
  nsCAutoString urlStr;
  urlStr += kNewsRootURI;
  urlStr += "/";
  urlStr += hostname;
  urlStr += "/";
  urlStr += messageId;
  urlStr += "?cancel";

#ifdef DEBUG_NEWS
  printf("attempt to cancel the message (key,ID,cancel url): (%d,%s,%s)\n", key, messageId.GetBuffer(),urlStr.GetBuffer());
#endif  

  nsCAutoString newsgroupNameStr(newsgroupname);
  nsCOMPtr<nsIURI> url;
  rv = ConstructNntpUrl(urlStr, newsgroupNameStr, key, aUrlListener,  getter_AddRefs(url));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(url);
  if (nntpUrl)
	nntpUrl->SetNewsAction(nsINntpUrl::ActionCancelArticle);
  rv = RunNewsUrl(url, aConsumer);  

  if (aURL)
  {
	*aURL = url;
	NS_IF_ADDREF(*aURL);
  }
  return rv; 
}

NS_IMETHODIMP nsNntpService::GetScheme(char * *aScheme)
{
	nsresult rv = NS_OK;
	if (aScheme)
		*aScheme = PL_strdup("news");
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 
}

NS_IMETHODIMP nsNntpService::GetDefaultPort(PRInt32 *aDefaultPort)
{
	nsresult rv = NS_OK;
	if (aDefaultPort)
		*aDefaultPort = NEWS_PORT;
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 	
}

NS_IMETHODIMP nsNntpService::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **_retval)
{
	nsresult rv = NS_OK;

	nsCOMPtr<nsINntpUrl> nntpUrl;
	rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, NS_GET_IID(nsINntpUrl), getter_AddRefs(nntpUrl));
	if (NS_FAILED(rv)) return rv;
	nntpUrl->SetNewsAction(nsINntpUrl::ActionDisplayArticle);

	nntpUrl->QueryInterface(NS_GET_IID(nsIURI), (void **) _retval);

	// don't worry this cast is really okay...there'a bug in XPIDL compiler that is preventing
	// a "cont char *" in paramemter for uri SetSpec...
	(*_retval)->SetSpec((char *) aSpec);
	(*_retval)->SetPort(NEWS_PORT);
	return rv;
}

NS_IMETHODIMP nsNntpService::NewChannel(const char *verb, nsIURI *aURI, nsILoadGroup *aGroup,
                                        nsIEventSinkGetter *eventSinkGetter, nsIURI* originalURI,
                                        nsIChannel **_retval)
{
	nsresult rv = NS_OK;
	nsNNTPProtocol *nntpProtocol = new nsNNTPProtocol(aURI);
	if (!nntpProtocol) return NS_ERROR_OUT_OF_MEMORY;
  
	rv = nntpProtocol->Initialize();
    if (NS_FAILED(rv)) return rv;
	nntpProtocol->SetLoadGroup(aGroup);
    return nntpProtocol->QueryInterface(NS_GET_IID(nsIChannel), (void **) _retval);
}

NS_IMETHODIMP
nsNntpService::SetDefaultLocalPath(nsIFileSpec *aPath)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kCPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->SetFilePref(PREF_MAIL_ROOT_NNTP, aPath, PR_FALSE /* set default */);
    return rv;
}

NS_IMETHODIMP
nsNntpService::GetDefaultLocalPath(nsIFileSpec ** aResult)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kCPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->GetFilePref(PREF_MAIL_ROOT_NNTP, aResult);
    if (NS_SUCCEEDED(rv)) return rv;

    NS_WITH_SERVICE(nsIFileLocator, locator, kFileLocatorCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = locator->GetFileLocation(nsSpecialFileSpec::App_NewsDirectory50, aResult);
    if (NS_FAILED(rv)) return rv; 

    rv = SetDefaultLocalPath(*aResult);
    return rv;
}
    
NS_IMETHODIMP
nsNntpService::GetServerIID(nsIID* *aServerIID)
{
    *aServerIID = new nsIID(NS_GET_IID(nsINntpIncomingServer));
    return NS_OK;
}
