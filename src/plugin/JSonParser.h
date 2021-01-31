
#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include <sstream>
#include <regex>
#include <optional>

class JSONParser
{
public:
  enum DataType{
    DataTypeBuffer,
    DataTypeFile
  };

    JSONParser() = default;

    JSONParser(const std::string& data, DataType type = DataTypeBuffer)
    {
        switch(type){
          case DataTypeBuffer:
            readFromBuffer(data);
          break;
          case DataTypeFile:
            readFromFile(data);
          break;
          default:
            throw std::invalid_argument("Wrong type of data used");
        }
    }

    JSONParser(const boost::property_tree::ptree& jsonTree):mJSONTree(jsonTree)
    {}

   template<typename T>
   T get(const std::string name) const
   {
       return mJSONTree.get<T>(name);
   }

   template<typename T>
   T get(const std::string name, T defaultValue) const
   {
       return mJSONTree.get(name, defaultValue);
   }

   template<typename T>
   std::optional<T>     getOptional(const std::string name) const
   {
        try{
            return get<T>(name);
        }catch(const std::exception&){
            return {};
        }
   }

   template<typename T>
   void put(const std::string name, T value)
   {
       mJSONTree.put(name, value);
   }

   void addObject(const std::string& name, JSONParser& object)
   {
        mJSONTree.push_back(std::make_pair(name, object.getRawTree()));
   }

   void addObject(const std::string& name, const std::string& jsonObject)
   {
        JSONParser object(jsonObject);
        addObject(name, object);
   }

   void addChild(const std::string& name, JSONParser& object)
   {
        mJSONTree.add_child(name, object.getRawTree());
   }

   template<typename T>
   std::vector<T> getArrayOfValues(const std::string name) const
   {
       std::vector<T> vec;
       try{
           for(auto& item: mJSONTree.get_child(name)){
               vec.push_back(item.second.get_value<T>());
           }
       }catch(const std::exception& exc){

       }

       return vec;
   }

   std::vector<JSONParser> getArrayOfObjects(const std::string name) const
   {
       std::vector<JSONParser> vec;
       try{
            for(auto& object: mJSONTree.get_child(name)){
                vec.push_back(JSONParser(object.second));
            }
       }catch(const std::exception& exc){

       }
       return vec;
   }

   const std::optional<JSONParser> getObject(const std::string name) const
   {
        try{
            return JSONParser(mJSONTree.get_child(name));
        }catch(const std::exception& exc){
            return {};
        }
   }

   JSONParser getObjectThrow(const std::string name) const
   {
       return JSONParser(mJSONTree.get_child(name));
   }

   std::string getJSON() const
   {
        std::stringstream ss;
        boost::property_tree::write_json(ss, mJSONTree, false);

        return cleanOutput(ss);
   }

   boost::property_tree::ptree& getRawTree()
   {
    return mJSONTree;
   }

   void writeToFile(const std::string& fileName)
   {
       boost::property_tree::write_json(fileName, mJSONTree);
   }

private:
    std::string cleanOutput(std::stringstream& ss) const
    {
        std::string result;
        std::regex removeQuotationMarks("\\\"(-{0,1}[0-9]+\\.{0,1}[0-9]*)\\\"");

        result = std::regex_replace(ss.str(), removeQuotationMarks, "$1");

        return result;
    }

    void readFromBuffer(const std::string& data)
    {
        std::stringstream ss;
        ss << data;
        boost::property_tree::read_json(ss, mJSONTree);
    }

    void readFromFile(const std::string& fileName)
    {
        boost::property_tree::read_json(fileName, mJSONTree);
    }


   boost::property_tree::ptree mJSONTree;
};


#endif // JSONPARSER_H
