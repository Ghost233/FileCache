#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"

#include "FileCache.h"

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
    FileCache::sharedInstant()->addFileAsync("HelloWorld.png", this, callfuncO_selector(HelloWorld::abc));
    
    return true;
}

void HelloWorld::abc(CCObject* string)
{
    CCString *tempString = (CCString*) string;
    
    CCLOG("%d", tempString->length());
    
    CCTexture2D *texture = new CCTexture2D();
    CCImage *image = new CCImage();
    image->initWithImageData((void * ) tempString->getCString(), tempString->length());
    texture->initWithImage(image);
    CCSprite *sprite = CCSprite::createWithTexture(texture);
    this->addChild(sprite);
    sprite->setPosition(ccp(500, 500));
}

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCDirector::sharedDirector()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
