/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "PocketSphinxSpeechRecognitionService.h"
#include "nsIFile.h"

#include "SpeechRecognition.h"
#include "SpeechRecognitionAlternative.h"
#include "SpeechRecognitionResult.h"
#include "SpeechRecognitionResultList.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsMemory.h"

extern "C"
{
  #include "pocketsphinx/pocketsphinx.h"
  #include "sphinxbase/sphinx_config.h"
  #include "sphinxbase/jsgf.h"

}

namespace mozilla {

  using namespace dom;


class DecodeResultTask : public nsRunnable
{
public:

  DecodeResultTask( const nsString& hypstring , WeakPtr<dom::SpeechRecognition> recognition)
    : mResult(hypstring)
    , mRecognition(recognition)
    , mWorkerThread(do_GetCurrentThread())
  {
    MOZ_ASSERT(!NS_IsMainThread()); // This should be running on the worker thread
  }

  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread()); // This method is supposed to run on the main thread!


    // Declare javascript result events
    nsRefPtr<SpeechEvent> event = new SpeechEvent(mRecognition, SpeechRecognition::EVENT_RECOGNITIONSERVICE_FINAL_RESULT);
    SpeechRecognitionResultList* resultList = new SpeechRecognitionResultList(mRecognition);
    SpeechRecognitionResult* result = new SpeechRecognitionResult(mRecognition);
    SpeechRecognitionAlternative* alternative = new SpeechRecognitionAlternative(mRecognition);

    alternative->mTranscript =  mResult ;
    alternative->mConfidence = 100;

    result->mItems.AppendElement(alternative);
    resultList->mItems.AppendElement(result);

    event->mRecognitionResultList = resultList;
    NS_DispatchToMainThread(event);

    // If we don't destroy the thread when we're done with it, it will hang around forever... bad!
    // But thread->Shutdown must be called from the main thread, not from the thread itself.
    return mWorkerThread->Shutdown();
  }

private:
  nsString mResult;
  nsCOMPtr<nsIThread> mWorkerThread;
  WeakPtr<dom::SpeechRecognition> mRecognition;
};

class DecodeTask : public nsRunnable
{

public:
  DecodeTask(WeakPtr<dom::SpeechRecognition> recogntion , std::vector<int16_t>   audiovector , ps_decoder_t * ps)
    : mRecognition(recogntion)
    , mAudiovector(audiovector)
    , mPs(ps)
  { }

  NS_IMETHOD Run() {


    char const *hyp, *uttid;
    int rv;
    int32 score;
    nsString hypoValue;

    rv = ps_start_utt(mPs, "goforward");
    rv = ps_process_raw(mPs, &mAudiovector[0], mAudiovector.size(), FALSE, FALSE);

    rv = ps_end_utt(mPs);
    if (rv >= 0)
    {
      hyp = ps_get_hyp(mPs, &score, &uttid);
      if (hyp == NULL)
      {
        hypoValue.AssignASCII("ERROR");
      }
      else
      {
        hypoValue.AssignASCII(hyp);
      }
    }


    nsCOMPtr<nsIRunnable> resultrunnable = new DecodeResultTask(hypoValue , mRecognition );
    return NS_DispatchToMainThread(resultrunnable);
  }

private:
  WeakPtr<dom::SpeechRecognition> mRecognition;
  ps_decoder_t * mPs;
  std::vector<int16_t>  mAudiovector;

};

  NS_IMPL_ISUPPORTS(PocketSphinxSpeechRecognitionService, nsISpeechRecognitionService, nsIObserver)

  PocketSphinxSpeechRecognitionService::PocketSphinxSpeechRecognitionService()
  {
    mSpeexState = nullptr;

    // get root folder
    nsCOMPtr<nsIFile> tmpFile;
    nsString aStringAMPath; // am folder
    nsString aStringDictPath; // dict folder

    NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(tmpFile));
    #if  defined(XP_WIN) // for some reason, on windows NS_GRE_DIR is not bin root, but bin/browser
    tmpFile->AppendRelativePath(NS_LITERAL_STRING("..") );    
    #endif
    tmpFile->AppendRelativePath(NS_LITERAL_STRING("models") );
    tmpFile->AppendRelativePath(NS_LITERAL_STRING("en-us-semi") );
    tmpFile->GetPath(aStringAMPath);

    NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(tmpFile));
    #if  defined(XP_WIN)  // for some reason, on windows NS_GRE_DIR is not bin root, but bin/browser
    tmpFile->AppendRelativePath(NS_LITERAL_STRING("..") );    
    #endif    
    tmpFile->AppendRelativePath(NS_LITERAL_STRING( "models") ); //
    tmpFile->AppendRelativePath(NS_LITERAL_STRING( "dict") ); //
    tmpFile->AppendRelativePath(NS_LITERAL_STRING( "cmu07a.dic") ); //   
    tmpFile->GetPath(aStringDictPath);


    // FOR B2G PATHS HARDCODED (APPEND /DATA ON THE BEGINING, FOR DESKTOP, ONLY MODELS/ RELATIVE TO ROOT
    config = cmd_ln_init(NULL, ps_args(), TRUE,
               "-hmm", ToNewUTF8String(aStringAMPath)  , // acoustic model
               "-dict",  ToNewUTF8String(aStringDictPath),
               NULL);
     if (config == NULL)
     {
       decodersane = false;
     }
     else
     {
       ps = ps_init(config);
       if (ps == NULL)
       {
         decodersane = false;
       }
       else
       {
         decodersane = true;
       }
     }

     grammarsane = false;
  }

  PocketSphinxSpeechRecognitionService::~PocketSphinxSpeechRecognitionService()
  {
    if (config)
      free(config);
    if (ps)
      free(ps);

    mSpeexState = nullptr;
  }

  // CALL START IN JS FALLS HERE
  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::Initialize(WeakPtr<SpeechRecognition> aSpeechRecognition)
  {
    if (!decodersane || !grammarsane)
    {
      return NS_ERROR_NOT_INITIALIZED;
    }
    else
    {
      audiovector.clear();

      if (mSpeexState)
        mSpeexState = nullptr;

      mRecognition = aSpeechRecognition;
      nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
      obs->AddObserver(this, SPEECH_RECOGNITION_TEST_EVENT_REQUEST_TOPIC, false);
      obs->AddObserver(this, SPEECH_RECOGNITION_TEST_END_TOPIC, false);
      return NS_OK;
    }
  }

  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::ProcessAudioSegment(AudioSegment* aAudioSegment, int32_t aSampleRate)
  {
    if (!mSpeexState) {
        mSpeexState = speex_resampler_init(1,  aSampleRate, 16000,  SPEEX_RESAMPLER_QUALITY_MAX  ,  nullptr);
    }
    aAudioSegment->ResampleChunks(mSpeexState, aSampleRate , 16000);

    AudioSegment::ChunkIterator iterator(*aAudioSegment);

    while(!iterator.IsEnded())
    {
       mozilla::AudioChunk& chunk = *(iterator);
       MOZ_ASSERT(chunk.mBuffer);
       const int16_t* buf = static_cast<const int16_t*>(chunk.mChannelData[0]);

       for(int i=0; i<iterator->mDuration; i++) {
           audiovector.push_back( (int16_t)buf[i] );
       }
       iterator.Next();
   }
    return NS_OK;
  }

  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::SoundEnd()
  {
    speex_resampler_destroy(mSpeexState);
    mSpeexState= nullptr;

    // To create a new thread, get the thread manager
    nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
    nsCOMPtr<nsIThread> mythread;
    nsresult rv = tm->NewThread(0, 0, getter_AddRefs(mythread));
    if (NS_FAILED(rv)) {
       // In case of failure, call back immediately with an empty string which indicates failure
       return NS_OK;
    }

    nsCOMPtr<nsIRunnable> r = new DecodeTask(mRecognition , audiovector , ps );
    mythread->Dispatch(r, nsIEventTarget::DISPATCH_NORMAL);;



    return NS_OK;
  }

  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::SetGrammarList(SpeechGrammarList *aSpeechGramarList)
  {

    const char * mGram = NULL;

    if (!decodersane)
    {
      grammarsane = false;
    }
    else if (aSpeechGramarList)
    {
       mGram = aSpeechGramarList->mGram;
       int result = ps_set_jsgf_string(ps, "name" , mGram);
       ps_set_search(ps, "name");

       if (result != 0)
       {
         grammarsane = false;
       }
       else
       {
         grammarsane = true;
       }
    }
    else
    {
      grammarsane = false;
    }

    return  grammarsane ? NS_OK : NS_ERROR_NOT_INITIALIZED;

  }

  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::Abort()
  {
    return NS_OK;
  }

  NS_IMETHODIMP
  PocketSphinxSpeechRecognitionService::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* aData)
  {

    MOZ_ASSERT(mRecognition->mTestConfig.mFakeRecognitionService,
               "Got request to fake recognition service event, but "
               TEST_PREFERENCE_FAKE_RECOGNITION_SERVICE " is not set");

    if (!strcmp(aTopic, SPEECH_RECOGNITION_TEST_END_TOPIC)) {
      nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
      obs->RemoveObserver(this, SPEECH_RECOGNITION_TEST_EVENT_REQUEST_TOPIC);
      obs->RemoveObserver(this, SPEECH_RECOGNITION_TEST_END_TOPIC);

      return NS_OK;
    }

    const nsDependentString eventName = nsDependentString(aData);

    if (eventName.EqualsLiteral("EVENT_RECOGNITIONSERVICE_ERROR")) {
      mRecognition->DispatchError(SpeechRecognition::EVENT_RECOGNITIONSERVICE_ERROR,
                                  SpeechRecognitionErrorCode::Network, // TODO different codes?
                                  NS_LITERAL_STRING("RECOGNITIONSERVICE_ERROR test event"));

    } else if (eventName.EqualsLiteral("EVENT_RECOGNITIONSERVICE_FINAL_RESULT")) {
      nsRefPtr<SpeechEvent> event =
        new SpeechEvent(mRecognition,
                        SpeechRecognition::EVENT_RECOGNITIONSERVICE_FINAL_RESULT);

      event->mRecognitionResultList = BuildMockResultList();
      NS_DispatchToMainThread(event);
    }

    return NS_OK;
    }

    SpeechRecognitionResultList* PocketSphinxSpeechRecognitionService::BuildMockResultList()
    {
      SpeechRecognitionResultList* resultList = new SpeechRecognitionResultList(mRecognition);
      SpeechRecognitionResult* result = new SpeechRecognitionResult(mRecognition);
      SpeechRecognitionAlternative* alternative = new SpeechRecognitionAlternative(mRecognition);

      alternative->mTranscript = NS_LITERAL_STRING("Mock final result");
      alternative->mConfidence = 0.0f;

      result->mItems.AppendElement(alternative);
      resultList->mItems.AppendElement(result);

      return resultList;
    }

} // namespace mozilla
