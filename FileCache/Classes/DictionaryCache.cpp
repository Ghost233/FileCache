//
//  DictionaryCache.cpp
//  FileCache
//
//  Created by Ghost on 13-5-27.
//
//

#include "DictionaryCache.h"
#include "FileCache.h"

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
    CCDictionary *temp = CCDictionary::createWithContentsOfFileThreadSafe(path);
    m_pDictionaryCache->setObject(temp, path);
    return temp;
}

CCDictionary* DictionaryCache::addDictionaryWithString(CCString *data, const char *name)
{
    CCDictionary *dict = FileCache::sharedInstant()->parseDictionary(data->getCString(), data->length());
    m_pDictionaryCache->setObject(dict, name);
    return dict;
}

void DictionaryCache::addDictionaryAsync(const char *path, CCObject *target, SEL_CallFuncO selector)
{
    FileCache::sharedInstant()->addFileAsync(path, this, callfuncO_selector(DictionaryCache::addDictionaryAsyncCallBack), CCCallFuncO::create(target, selector, NULL));
}

void DictionaryCache::addDictionaryAsyncCallBack(CCObject *object)
{
    CCCallFuncO *call = (CCCallFuncO*)object;
    CCDictionary *dictionary = (CCDictionary*) call->getObject();
    CCString *data = (CCString*) dictionary->objectForKey("data");
    CCString *name = (CCString*) dictionary->objectForKey("name");
    CCDictionary *csv = FileCache::sharedInstant()->parseDictionary(data->getCString(), data->length());
    m_pDictionaryCache->setObject(csv, name->m_sString);
    call->setObject(csv);
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
