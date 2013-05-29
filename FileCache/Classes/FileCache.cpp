//
//  FileCache.cpp
//  Salamender
//
//  Created by Ghost on 13-5-17.
//
//

#include "FileCache.h"
#include "CCSAXParser.h"
#include <stack>

typedef enum
{
    SAX_NONE = 0,
    SAX_KEY,
    SAX_DICT,
    SAX_INT,
    SAX_REAL,
    SAX_STRING,
    SAX_ARRAY
}CCSAXStateTemp;

typedef enum
{
    SAX_RESULT_NONE = 0,
    SAX_RESULT_DICT,
    SAX_RESULT_ARRAY
}CCSAXResultTemp;

class CCDictMakerTemp : public CCSAXDelegator
{
public:
    CCSAXResultTemp m_eResultType;
    CCArray* m_pRootArray;
    CCDictionary *m_pRootDict;
    CCDictionary *m_pCurDict;
    std::stack<CCDictionary*> m_tDictStack;
    std::string m_sCurKey;   ///< parsed key
    std::string m_sCurValue; // parsed value
    CCSAXStateTemp m_tState;
    CCArray* m_pArray;
    
    std::stack<CCArray*> m_tArrayStack;
    std::stack<CCSAXStateTemp>  m_tStateStack;
    
public:
    CCDictMakerTemp()
    : m_eResultType(SAX_RESULT_NONE),
    m_pRootArray(NULL),
    m_pRootDict(NULL),
    m_pCurDict(NULL),
    m_tState(SAX_NONE),
    m_pArray(NULL)
    {
    }
    
    ~CCDictMakerTemp()
    {
    }
    
    CCDictionary* dictionaryWithContentsOfFile(const char *pFileName)
    {
        m_eResultType = SAX_RESULT_DICT;
        CCSAXParser parser;
        
        if (false == parser.init("UTF-8"))
        {
            return NULL;
        }
        parser.setDelegator(this);
        
        parser.parse(pFileName);
        return m_pRootDict;
    }
    
    CCDictionary* dictionaryWithString(const char *data, unsigned int length)
    {
        m_eResultType = SAX_RESULT_DICT;
        CCSAXParser parser;
        
        if (false == parser.init("UTF-8"))
        {
            return NULL;
        }
        parser.setDelegator(this);
        
        parser.parse(data, length);
        return m_pRootDict;
    }
    
    CCArray* arrayWithContentsOfFile(const char* pFileName)
    {
        m_eResultType = SAX_RESULT_ARRAY;
        CCSAXParser parser;
        
        if (false == parser.init("UTF-8"))
        {
            return NULL;
        }
        parser.setDelegator(this);
        
        parser.parse(pFileName);
        return m_pArray;
    }
    
    void startElement(void *ctx, const char *name, const char **atts)
    {
        CC_UNUSED_PARAM(ctx);
        CC_UNUSED_PARAM(atts);
        std::string sName((char*)name);
        if( sName == "dict" )
        {
            m_pCurDict = new CCDictionary();
            if(m_eResultType == SAX_RESULT_DICT && m_pRootDict == NULL)
            {
                // Because it will call m_pCurDict->release() later, so retain here.
                m_pRootDict = m_pCurDict;
                m_pRootDict->retain();
            }
            m_tState = SAX_DICT;
            
            CCSAXStateTemp preState = SAX_NONE;
            if (! m_tStateStack.empty())
            {
                preState = m_tStateStack.top();
            }
            
            if (SAX_ARRAY == preState)
            {
                // add the dictionary into the array
                m_pArray->addObject(m_pCurDict);
            }
            else if (SAX_DICT == preState)
            {
                // add the dictionary into the pre dictionary
                CCAssert(! m_tDictStack.empty(), "The state is wrong!");
                CCDictionary* pPreDict = m_tDictStack.top();
                pPreDict->setObject(m_pCurDict, m_sCurKey.c_str());
            }
            
            m_pCurDict->release();
            
            // record the dict state
            m_tStateStack.push(m_tState);
            m_tDictStack.push(m_pCurDict);
        }
        else if(sName == "key")
        {
            m_tState = SAX_KEY;
        }
        else if(sName == "integer")
        {
            m_tState = SAX_INT;
        }
        else if(sName == "real")
        {
            m_tState = SAX_REAL;
        }
        else if(sName == "string")
        {
            m_tState = SAX_STRING;
        }
        else if (sName == "array")
        {
            m_tState = SAX_ARRAY;
            m_pArray = new CCArray();
            if (m_eResultType == SAX_RESULT_ARRAY && m_pRootArray == NULL)
            {
                m_pRootArray = m_pArray;
                m_pRootArray->retain();
            }
            CCSAXStateTemp preState = SAX_NONE;
            if (! m_tStateStack.empty())
            {
                preState = m_tStateStack.top();
            }
            
            if (preState == SAX_DICT)
            {
                m_pCurDict->setObject(m_pArray, m_sCurKey.c_str());
            }
            else if (preState == SAX_ARRAY)
            {
                CCAssert(! m_tArrayStack.empty(), "The state is wrong!");
                CCArray* pPreArray = m_tArrayStack.top();
                pPreArray->addObject(m_pArray);
            }
            m_pArray->release();
            // record the array state
            m_tStateStack.push(m_tState);
            m_tArrayStack.push(m_pArray);
        }
        else
        {
            m_tState = SAX_NONE;
        }
    }
    
    void endElement(void *ctx, const char *name)
    {
        CC_UNUSED_PARAM(ctx);
        CCSAXStateTemp curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
        std::string sName((char*)name);
        if( sName == "dict" )
        {
            m_tStateStack.pop();
            m_tDictStack.pop();
            if ( !m_tDictStack.empty())
            {
                m_pCurDict = m_tDictStack.top();
            }
        }
        else if (sName == "array")
        {
            m_tStateStack.pop();
            m_tArrayStack.pop();
            if (! m_tArrayStack.empty())
            {
                m_pArray = m_tArrayStack.top();
            }
        }
        else if (sName == "true")
        {
            CCString *str = new CCString("1");
            if (SAX_ARRAY == curState)
            {
                m_pArray->addObject(str);
            }
            else if (SAX_DICT == curState)
            {
                m_pCurDict->setObject(str, m_sCurKey.c_str());
            }
            str->release();
        }
        else if (sName == "false")
        {
            CCString *str = new CCString("0");
            if (SAX_ARRAY == curState)
            {
                m_pArray->addObject(str);
            }
            else if (SAX_DICT == curState)
            {
                m_pCurDict->setObject(str, m_sCurKey.c_str());
            }
            str->release();
        }
        else if (sName == "string" || sName == "integer" || sName == "real")
        {
            CCString* pStrValue = new CCString(m_sCurValue);
            
            if (SAX_ARRAY == curState)
            {
                m_pArray->addObject(pStrValue);
            }
            else if (SAX_DICT == curState)
            {
                m_pCurDict->setObject(pStrValue, m_sCurKey.c_str());
            }
            
            pStrValue->release();
            m_sCurValue.clear();
        }
        
        m_tState = SAX_NONE;
    }
    
    void textHandler(void *ctx, const char *ch, int len)
    {
        CC_UNUSED_PARAM(ctx);
        if (m_tState == SAX_NONE)
        {
            return;
        }
        
        CCSAXStateTemp curState = m_tStateStack.empty() ? SAX_DICT : m_tStateStack.top();
        CCString *pText = new CCString(std::string((char*)ch,0,len));
        
        switch(m_tState)
        {
            case SAX_KEY:
                m_sCurKey = pText->getCString();
                break;
            case SAX_INT:
            case SAX_REAL:
            case SAX_STRING:
            {
                if (curState == SAX_DICT)
                {
                    CCAssert(!m_sCurKey.empty(), "key not found : <integer/real>");
                }
                
                m_sCurValue.append(pText->getCString());
            }
                break;
            default:
                break;
        }
        pText->release();
    }
};

CCDictionary* FileCache::parseDictionary(const char *data, unsigned int length)
{
    CCDictMakerTemp tMaker;
    CCDictionary *dict = tMaker.dictionaryWithString(data, length);
    return dict;
}

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

void FileCache::removeCSV(const char * filename)
{
    m_pDataCache->removeObjectForKey(filename);
}
