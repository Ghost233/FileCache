//
//  CSVCache.h
//  FileCache
//
//  Created by Ghost on 13-5-23.
//
//

#ifndef __FileCache__CSVCache__
#define __FileCache__CSVCache__

#include "cocos2d.h"
#include <queue>
#include <vector>

USING_NS_CC;
using namespace std;

class CSVCache : public CCObject
{
public:
	CSVCache(void);
	~CSVCache(void);
    
	static CSVCache* sharedInstant();
    static void purgeInstant();
    
    virtual bool init();
    
    CCDictionary* addCSV(const char *path);
    CCDictionary* addCSVWithString(CCString *data, const char *name);
    void addCSVAsync(const char *path, CCObject *target, SEL_CallFuncO selector);
    void addCSVAsyncCallBack(CCObject *object);
    CCDictionary* getCSV(const char *path);
        
    void removeAllCache();
    void removeData(const char * filename);
    
    static CCDictionary* parseCSV(CCString *data);
    static CCDictionary* parseCSVThreadSafeWithRetain(CCString *data);
    static CCArray* splitToCCArray(CCString *source, string key);
    static CCArray* splitToCCArrayThreadSafeWithRetain(CCString *source, string key);
    static vector<string> split(const string& src, string delimit, string null_subst="");
    static CCString* substr(CCString *source, int location, int length);
    static CCString* substrThreadSafeWithRetain(CCString *source, int location, int length);
    
private:
    CCDictionary* m_pCSVCache;
};

#endif /* defined(__FileCache__CSVCache__) */
