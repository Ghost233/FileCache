//
//  FileCache.h
//  Salamender
//
//  Created by Ghost on 13-5-17.
//
//


#ifndef _FILECAHCE_
#define _FILECAHCE_

#include "cocos2d.h"
#include <queue>

USING_NS_CC;
using namespace std;

class FileCache : public CCObject
{
public:
	FileCache(void);
	~FileCache(void);

	static FileCache* sharedInstant();
    static void purgeInstant();
    
    virtual bool init();
    
    CCString* addFile(const char *path);
    CCString* getFileWithoutCache(const char *path);

    void addFileAsync(const char *path, CCObject *target, SEL_CallFuncO selector, CCCallFuncO *call = NULL);
    void addFileAsyncCallBack(float dt);
    CCString* getFile(const char *path);
    
    CCDictionary* parseDictionary(const char *data, unsigned int length);
    
    void removeAllCache();
    void removeCSV(const char * filename);
    
private:
    CCDictionary* m_pDataCache;
};

#endif