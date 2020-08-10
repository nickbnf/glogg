#ifndef BASETESTHANDLER_H
#define BASETESTHANDLER_H
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include "JSonParser.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <boost/core/demangle.hpp>

using namespace std;

class Handler
{
    public:
    Handler() = default;
    virtual ~Handler() = default;

    virtual void onRelease();

    template<typename T>
    static void addPythonTemplate(vector<JSONParser>& jh, const string& className, T* hTemplate)
    {
        hTemplate->className = className;
        hTemplate->mInstanceName = hTemplate->className;

        cout << "\n# Loading Handler: [" << hTemplate->className << "] #\n";

        auto it = find_if(jh.begin(), jh.end(), [&](const JSONParser& item){
               return item.get<string>("name") == hTemplate->className; });

        if(it == jh.end()){
            cout << "! Handler: [" << hTemplate->className << "] not found\n";
        } else {
            hTemplate->loadFromJson(*it);
            jh.erase(it);
        }

        //mHandlers.insert_or_assign(hTemplate->mInstanceName, create(hTemplate));
    }

    virtual void loadFromJson(const JSONParser &jp);

    void setProperties(const JSONParser& properties);

    static Handler& getHandler(const string& className);

    const string& getInstanceName();
    vector<string> mUsingProperties;

    string getRawProperties(){ return mProperties.getJSON(); }
    vector<string> getTriggers() const;


    static void initSharedData(const JSONParser &jp);

    //optional<Timer> mTimer;
protected:
    static map<string, shared_ptr<Handler>> mHandlers;

    string className;
    string mInstanceName;
    int mCurrentTrigger = 0;
    JSONParser mProperties;
    int mInputFileDescriptor = 0;
};




#endif // BASETESTHANDLER_H