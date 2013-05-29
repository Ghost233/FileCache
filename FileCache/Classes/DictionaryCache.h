//
//  DictionaryCache.h
//  FileCache
//
//  Created by Ghost on 13-5-27.
//
//

#ifndef __FileCache__DictionaryCache__
#define __FileCache__DictionaryCache__

#include "cocos2d.h"
#include <queue>
#include <vector>

USING_NS_CC;
using namespace std;

class DictionaryCache : public CCObject
{
public:
	DictionaryCache(void);
	~DictionaryCache(void);
    
	static DictionaryCache* sharedInstant();
    static void purgeInstant();
    
    virtual bool init();
    
    CCDictionary* addDictionary(const char *path);
    CCDictionary* addDictionaryWithString(CCString *data, const char *name);
    void addDictionaryAsync(const char *path, CCObject *target, SEL_CallFuncO selector);
    void addDictionaryAsyncCallBack(CCObject *object);
    CCDictionary* getDictionary(const char *path);
    
    void removeAllCache();
    void removeData(const char * filename);
    
private:
    CCDictionary* m_pDictionaryCache;
    CCDictionary* parseCSV(CCString *data);
    vector<string> split(const string& src, string delimit, string null_subst="");
    CCArray* splitToCCArray(CCString *source, string key);
    CCString* substr(CCString *source, int location, int length);
};

#endif /* defined(__FileCache__DictionaryCache__) */
