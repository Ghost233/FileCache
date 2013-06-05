//
//  DictionaryCache.cpp
//  FileCache
//
//  Created by Ghost on 13-5-27.
//
//

#include "DictionaryCache.h"
#include "FileCache.h"
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

CCDictionary* DictionaryCache::parseDictionary(CCString *data)
{
    CCDictionary *temp = DictionaryCache::parseDictionaryThreadSafeWithRetain(data);
    temp->autorelease();
    return temp;
}

CCDictionary* DictionaryCache::parseDictionaryThreadSafeWithRetain(CCString *data)
{
    CCDictMakerTemp tMaker;
    CCDictionary *temp = tMaker.dictionaryWithString(data->getCString(), data->length());
    return temp;
}

static DictionaryCache* s_sharedInstant;

DictionaryCache::DictionaryCache(void)
{
    
}

DictionaryCache::~DictionaryCache(void)
{
    CC_SAFE_RELEASE_NULL(m_pDictionaryCache);
}

DictionaryCache* DictionaryCache::sharedInstant()
{
    if (s_sharedInstant == NULL)
	{
		s_sharedInstant = new DictionaryCache();
        s_sharedInstant->init();
	}
	return s_sharedInstant;
}

void DictionaryCache::purgeInstant()
{
    CC_SAFE_RELEASE_NULL(s_sharedInstant);
}

bool DictionaryCache::init()
{
    m_pDictionaryCache = CCDictionary::create();
    m_pDictionaryCache->retain();
    
    return true;
}

CCDictionary* DictionaryCache::addDictionary(const char *path)
{
    CCDictionary *temp = (CCDictionary*) m_pDictionaryCache->objectForKey(path);
    if (temp) return temp;
    temp = CCDictionary::createWithContentsOfFileThreadSafe(path);
    m_pDictionaryCache->setObject(temp, path);
    return temp;
}

CCDictionary* DictionaryCache::addDictionaryWithString(CCString *data, const char *name)
{
    CCDictionary *temp = (CCDictionary*) m_pDictionaryCache->objectForKey(name);
    if (temp) return temp;
    CCDictMakerTemp tMaker;
    temp = tMaker.dictionaryWithString(data->getCString(), data->length());
    m_pDictionaryCache->setObject(temp, name);
    return temp;
}

void DictionaryCache::addDictionaryAsync(const char *path, CCObject *target, SEL_CallFuncO selector)
{
    CCDictionary *temp = (CCDictionary*) m_pDictionaryCache->objectForKey(path);
    if (temp)
    {
        CCCallFuncO *call = CCCallFuncO::create(target, selector, temp);
        call->execute();
        return;
    }
    FileCache::sharedInstant()->addFileAsync(path, this, callfuncO_selector(DictionaryCache::addDictionaryAsyncCallBack), CCCallFuncO::create(target, selector, NULL), DataTypeDictionray);
}

void DictionaryCache::addDictionaryAsyncCallBack(CCObject *object)
{
    CCCallFuncO *call = (CCCallFuncO*)object;
    CCDictionary *dictionary = (CCDictionary*) call->getObject();
    CCDictionary *data = (CCDictionary*) dictionary->objectForKey("data");
    CCString *name = (CCString*) dictionary->objectForKey("name");
    m_pDictionaryCache->setObject(data, name->m_sString);
    call->setObject(data);
    call->execute();
}

CCDictionary* DictionaryCache::getDictionary(const char *path)
{
    return (CCDictionary*) m_pDictionaryCache->objectForKey(path);
}

void DictionaryCache::removeAllCache()
{
    m_pDictionaryCache->removeAllObjects();
}

void DictionaryCache::removeDictionary(const char * filename)
{
    m_pDictionaryCache->removeObjectForKey(filename);
}
