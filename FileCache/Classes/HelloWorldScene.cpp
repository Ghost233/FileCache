#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"

#include "FileCache.h"
#include "DictionaryCache.h"
#include "CSVCache.h"

using namespace cocos2d;
using namespace CocosDenshion;

CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    cocos2d::CCLayer::init();
    CSVCache::sharedInstant()->addCSVAsync("aaa.csv", this, callfuncO_selector(HelloWorld::abc));
//    DictionaryCache::sharedInstant()->addDictionaryAsync("big15.plist", this, callfuncO_selector(HelloWorld::abc));
//    FileCache::sharedInstant()->addFileAsync("HelloWorld.png", this, callfuncO_selector(HelloWorld::abc));
    
    this->scheduleUpdate();
    
    return true;
}

void HelloWorld::abc(CCObject* string)
{
    akjdsf = (CCDictionary*) string;
    CCLOG("adf");
}

void HelloWorld::update(float delta)
{
    CCLOG("fghjk");
}

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCDirector::sharedDirector()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
