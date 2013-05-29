//
//  FileCache.cpp
//  Salamender
//
//  Created by Ghost on 13-5-17.
//
//

#include "FileCache.h"

typedef struct _AsyncStruct
{
    string            filename;
    CCObject    *target;
    SEL_CallFuncO        selector;
    CCCallFuncO        *call;
} AsyncStruct;

typedef struct _DataStruct
{
    AsyncStruct *asyncStruct;
    string *data;
} DataStruct;

static FileCache*  s_sharedInstant;

static pthread_t s_loadingThread;

static pthread_mutex_t      s_asyncStructQueueMutex;
static pthread_mutex_t      s_dataStructQueueMutex;

static queue<AsyncStruct*>* s_pAsyncStructQueue = NULL;
static queue<DataStruct*>* s_pDataStructQueue = NULL;

static unsigned long s_nAsyncRefCount = 0;

static bool need_quit = false;

FileCache::FileCache(void)
{

}

FileCache::~FileCache(void)
{
    m_pDataCache->release();
    need_quit = true;
}

FileCache* FileCache::sharedInstant()
{
	if (s_sharedInstant==NULL)
	{
		s_sharedInstant=new FileCache();
        s_sharedInstant->init();
	}
	return s_sharedInstant;
}

void FileCache::purgeInstant()
{
    CC_SAFE_RELEASE_NULL(s_sharedInstant);
}

bool FileCache::init()
{
    m_pDataCache = CCDictionary::create();
    m_pDataCache->retain();
    
    return true;
}

CCString* FileCache::addFile(const char *path)
{
    string filename(path);
    CCString *tempData = (CCString*) m_pDataCache->objectForKey(path);
    if (tempData != NULL)
    {
        return tempData;
    }
    
    unsigned long nSize = 0;
    unsigned char *pBuffer = CCFileUtils::sharedFileUtils()->getFileData(path, "rb", &nSize);
    
    CCString *tempString = CCString::createWithData(pBuffer, nSize);
    m_pDataCache->setObject(tempData, path);
    
    return tempString;
}

CCString* FileCache::getFileWithoutCache(const char *path)
{
    string filename(path);
    CCString *tempData = (CCString*) m_pDataCache->objectForKey(path);
    if (tempData != NULL)
    {
        return tempData;
    }
    
    unsigned long nSize = 0;
    unsigned char *pBuffer = CCFileUtils::sharedFileUtils()->getFileData(path, "rb", &nSize);
    
    CCString *tempString = CCString::createWithData(pBuffer, nSize);
    
    return tempString;
}

void FileCache::addFileAsync(const char *path, CCObject *target, SEL_CallFuncO selector, CCCallFuncO *call)
{
    string filename(path);
    CCString *tempData = (CCString*) m_pDataCache->objectForKey(path);
    if (tempData != NULL)
    {
        if (target && selector)
        {
            if (call)
            {
                CCDictionary *dic = CCDictionary::create();
                dic->setObject(tempData, "data");
                dic->setObject(CCString::create(path), "name");
                call->setObject(dic);
                (target->*selector)(call);
            }
            else
            {
                (target->*selector)(tempData);
            }
        }
        
        return;
    }
    
    if (s_pAsyncStructQueue == NULL)
    {
        s_pAsyncStructQueue = new queue<AsyncStruct*>();
        s_pDataStructQueue = new queue<DataStruct*>();
        
        pthread_mutex_init(&s_asyncStructQueueMutex, NULL);
        pthread_mutex_init(&s_dataStructQueueMutex, NULL);
        pthread_create(&s_loadingThread, NULL, asyncLoadFile, NULL);
        
        need_quit = false;
    }
    
    if (0 == s_nAsyncRefCount)
    {
        CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(FileCache::addFileAsyncCallBack), this, 0, false);
    }
    
    ++s_nAsyncRefCount;
    
    if (target)
    {
        target->retain();
    }
    
    if (call)
    {
        call->retain();
    }
    
    AsyncStruct *temp = new AsyncStruct();
    temp->filename = filename;
    temp->target = target;
    temp->selector = selector;
    temp->call = call;
    
    pthread_mutex_lock(&s_asyncStructQueueMutex);
    s_pAsyncStructQueue->push(temp);
    pthread_mutex_unlock(&s_asyncStructQueueMutex);
}

void FileCache::addFileAsyncCallBack(float dt)
{
    std::queue<DataStruct*> *dataStructQueue = s_pDataStructQueue;
    
    pthread_mutex_lock(&s_dataStructQueueMutex);
    if (dataStructQueue->empty())
    {
        pthread_mutex_unlock(&s_dataStructQueueMutex);
    }
    else
    {
        DataStruct *pDataStruct = dataStructQueue->front();
        dataStructQueue->pop();
        pthread_mutex_unlock(&s_dataStructQueueMutex);
        
        AsyncStruct *pAsyncStruct = pDataStruct->asyncStruct;
        string *string = pDataStruct->data;
        
        CCObject *target = pAsyncStruct->target;
        SEL_CallFuncO selector = pAsyncStruct->selector;
        const char* filename = pAsyncStruct->filename.c_str();
        CCCallFuncO *call = pAsyncStruct->call;
        
        CCString *tempString = CCString::create(*string);
        
        m_pDataCache->setObject(tempString, filename);
        
        if (target && selector && call)
        {
            CCDictionary *dic = CCDictionary::create();
            dic->setObject(tempString, "data");
            dic->setObject(CCString::create(filename), "name");
            call->setObject(dic);
            (target->*selector)(call);
            target->release();
            call->release();
        }
        else if (target && selector)
        {
            (target->*selector)(tempString);
            target->release();
        }
        
        delete pAsyncStruct;
        delete pDataStruct;
        
        --s_nAsyncRefCount;
        if (0 == s_nAsyncRefCount)
        {
            CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(FileCache::addFileAsyncCallBack), this);
        }
    }
}

CCString* FileCache::getFile(const char *path)
{
    CCString *tempString = (CCString*) m_pDataCache->objectForKey(path);
    return tempString;
}

void* FileCache::asyncLoadFile(void* data)
{
    AsyncStruct *pAsyncStruct = NULL;
    while (true)
    {
        CCThread thread;
        thread.createAutoreleasePool();
        
        std::queue<AsyncStruct*> *pQueue = s_pAsyncStructQueue;
        pthread_mutex_lock(&s_asyncStructQueueMutex);// get async struct from queue
        if (pQueue->empty())
        {
            pthread_mutex_unlock(&s_asyncStructQueueMutex);
            if (need_quit) {
                break;
            }
            else {
                continue;
            }
        }
        else
        {
            pAsyncStruct = pQueue->front();
            pQueue->pop();
            pthread_mutex_unlock(&s_asyncStructQueueMutex);
        }
        
        unsigned long nSize = 0;
        const char* fullpath = pAsyncStruct->filename.c_str();
        unsigned char *pBuffer = cocos2d::CCFileUtils::sharedFileUtils()->getFileData(fullpath, "rb", &nSize);
        
        pthread_mutex_lock(&s_dataStructQueueMutex);
        DataStruct *tempDataStruct = new DataStruct();
        tempDataStruct->asyncStruct = pAsyncStruct;
        string *tempString = new string((char*)pBuffer, nSize);
        nSize = tempString->length();
        tempDataStruct->data = tempString;
        s_pDataStructQueue->push(tempDataStruct);
        pthread_mutex_unlock(&s_dataStructQueueMutex);
    }
    
    if (s_pAsyncStructQueue != NULL)
    {
        delete s_pAsyncStructQueue;
        s_pAsyncStructQueue = NULL;
        delete s_pDataStructQueue;
        s_pDataStructQueue = NULL;
        
        pthread_mutex_destroy(&s_asyncStructQueueMutex);
    }
    
	return  NULL;
}

void FileCache::removeAllCache()
{
    m_pDataCache->removeAllObjects();
}

void FileCache::removeData(const char * filename)
{
    m_pDataCache->removeObjectForKey(filename);
}
