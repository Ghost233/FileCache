//
//  CSVCache.cpp
//  FileCache
//
//  Created by Ghost on 13-5-23.
//
//

#include "CSVCache.h"
#include "FileCache.h"

static CSVCache* s_sharedInstant;

CSVCache::CSVCache(void)
{
    
}

CSVCache::~CSVCache(void)
{
    CC_SAFE_RELEASE_NULL(m_pCSVCache);
}

CSVCache* CSVCache::sharedInstant()
{
    if (s_sharedInstant==NULL)
	{
		s_sharedInstant = new CSVCache();
        s_sharedInstant->init();
	}
	return s_sharedInstant;
}

void CSVCache::purgeInstant()
{
    CC_SAFE_RELEASE_NULL(s_sharedInstant);
}

bool CSVCache::init()
{
    m_pCSVCache = CCDictionary::create();
    m_pCSVCache->retain();
    
    return true;
}

CCDictionary* CSVCache::addCSV(const char *path)
{
    CCString *data = FileCache::sharedInstant()->getFileWithoutCache(path);
    CCDictionary *csv = this->parseCSV(data);
    m_pCSVCache->setObject(csv, path);
    return csv;
}

CCDictionary* CSVCache::addCSVWithString(CCString *data, const char *name)
{
    CCDictionary *csv = this->parseCSV(data);
    m_pCSVCache->setObject(csv, name);
    return csv;
}

void CSVCache::addCSVAsync(const char *path, CCObject *target, SEL_CallFuncO selector)
{
    FileCache::sharedInstant()->addFileAsync(path, this, callfuncO_selector(CSVCache::addCSVAsyncCallBack), CCCallFuncO::create(target, selector, NULL), DataTypeCSV);
}

void CSVCache::addCSVAsyncCallBack(CCObject *object)
{
    CCCallFuncO *call = (CCCallFuncO*)object;
    CCDictionary *dictionary = (CCDictionary*) call->getObject();
    CCString *data = (CCString*) dictionary->objectForKey("data");
    CCString *name = (CCString*) dictionary->objectForKey("name");
    m_pCSVCache->setObject(data, name->m_sString);
    call->setObject(data);
    call->execute();
}

CCDictionary* CSVCache::getCSV(const char *path)
{
    return (CCDictionary*)m_pCSVCache->objectForKey(path);
}

void CSVCache::removeAllCache()
{
    m_pCSVCache->removeAllObjects();
}

void CSVCache::removeData(const char * filename)
{
    m_pCSVCache->removeObjectForKey(filename);
}

CCDictionary* CSVCache::parseCSV(CCString* str)
{
    CCDictionary *temp = CSVCache::parseCSVThreadSafeWithRetain(str);
    temp->autorelease();
    return temp;
}

CCDictionary* CSVCache::parseCSVThreadSafeWithRetain(CCString* str)
{

    CCDictionary* data = new CCDictionary();
    
	CCArray* key = new CCArray();
    key->init();
	const char* tempSave = str->getCString();
	string tempStr;
	int iD = 1;
	bool isFirstLine = false;
	int i;
    
	for (i = 0;i < strlen(tempSave); ++ i)
	{
 		if ((int)tempSave[i] == 10 || (int)tempSave[i] == 13)
 		{
			if (!isFirstLine)
			{
                CCString *tempString = new CCString(tempStr);
				key = CSVCache::splitToCCArrayThreadSafeWithRetain(tempString, ";");
                CC_SAFE_RELEASE_NULL(tempString);
                
                for (int i = 0 ; i < key->count() ; ++i)
                {
                    CCString *tempString = (CCString*)key->objectAtIndex(i);
                    if (tempString->m_sString == "")
                    {
                        key->removeObjectAtIndex(i);
                        i--;
                    }
                }
				isFirstLine = true;
				tempStr.clear();
				i ++;
				continue;
			}
            
            CCString *tempString = new CCString(tempStr);
 			CCArray* temp = CSVCache::splitToCCArrayThreadSafeWithRetain(tempString, ";");
            CC_SAFE_RELEASE_NULL(tempString);
            CCString *tempString1 = (CCString*)temp->objectAtIndex(0);
            CCString *tempString2 = (CCString*)temp->objectAtIndex(1);
            tempStr.clear();
            if (tempString1->m_sString != "" || tempString2->m_sString != "")
            {
                CCDictionary* tempDic = new CCDictionary();
                
                for (int j = 0;j < key->count(); ++ j)
                {
                    tempDic->setObject(temp->objectAtIndex(j), ((CCString*)key->objectAtIndex(j))->getCString());
                }
                
                tempString = new CCString();
                tempString->initWithFormat("%d", iD ++);
                data->setObject(tempDic, tempString->m_sString);
                CC_SAFE_RELEASE_NULL(tempString);
                
                CC_SAFE_RELEASE_NULL(tempDic);
            }
            CC_SAFE_RELEASE_NULL(temp);
			i++;
 		}
		else
			tempStr += tempSave[i];
	}
    
	if (i == strlen(tempSave))
    {
        CCString *tempString = new CCString(tempStr);
        CCArray* temp = CSVCache::splitToCCArrayThreadSafeWithRetain(tempString, ";");
        CC_SAFE_RELEASE_NULL(tempString);
        tempStr.clear();
        CCString *tempString1 = (CCString*)temp->objectAtIndex(0);
        CCString *tempString2 = (CCString*)temp->objectAtIndex(1);
        if (tempString1->m_sString != "" || tempString2->m_sString != "")
        {
            CCDictionary* tempDic = new CCDictionary();
            
            for (int j = 0;j < key->count(); ++ j)
            {
                tempDic->setObject(temp->objectAtIndex(j), ((CCString*)key->objectAtIndex(j))->getCString());
            }
            
            tempString = new CCString();
            tempString->initWithFormat("%d", iD ++);
            data->setObject(tempDic, tempString->m_sString);
            CC_SAFE_RELEASE_NULL(tempString);
            CC_SAFE_RELEASE_NULL(tempDic);
        }
        CC_SAFE_RELEASE_NULL(temp);
    }
	return data;
}

typedef basic_string<char>::size_type S_T;
static const S_T _npos = -1;

vector<string> CSVCache::split(const string& src, string delimit, string null_subst)
{
    vector<string> v;
    v.clear();
    if( src.empty() || delimit.empty() )
        return v;
    S_T deli_len = delimit.size();
    long index = _npos, last_search_position = 0;
    while( (index=src.find(delimit, last_search_position))!=_npos )
    {
        if(index==last_search_position)
            v.push_back(null_subst);
        else
            v.push_back( src.substr(last_search_position, index-
                                    last_search_position) );
        last_search_position = index + deli_len;
    }
    string last_one = src.substr(last_search_position);
    v.push_back( last_one.empty()? null_subst:last_one );
    return v;
}

CCArray* CSVCache::splitToCCArray(CCString *source, string key)
{
    CCArray *temp = CSVCache::splitToCCArrayThreadSafeWithRetain(source, key);
    temp->autorelease();
    return temp;
}

CCArray* CSVCache::splitToCCArrayThreadSafeWithRetain(CCString *source, string key)
{
    string temp = source->m_sString;
    vector<string> array = split(temp, key);
    
    CCArray *arrayTemp = new CCArray;
    arrayTemp->init();
    
    for (int i = 0 ; i < array.size() ; ++i)
    {
        CCString *tempString = new CCString(array[i]);
        arrayTemp->addObject(tempString);
        CC_SAFE_RELEASE_NULL(tempString);
    }
    return arrayTemp;
}

CCString* CSVCache::substr(CCString *source, int location, int length)
{
    CCString *temp = CSVCache::substrThreadSafeWithRetain(source, location, length);
    temp->autorelease();
    return temp;
}

CCString* CSVCache::substrThreadSafeWithRetain(CCString *source, int location, int length)
{
    string temp = source->m_sString.substr(location, length);
    CCString *tempString = new CCString(temp);
    return tempString;
}
