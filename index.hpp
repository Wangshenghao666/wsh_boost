#pragma once 

#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_map>
#include<mutex>

#include"util.hpp"

namespace ns_index
{
  struct DocInfo
  {
    std::string title;   //文档标题
    std::string content; //去标签之后文档的内容
    std::string url;     //官网文档url
    uint64_t doc_id;     //文档id
  };

  struct InvertedElem
  {

    uint64_t doc_id;     //文档id
    std::string word;    //关键字
    int weight;          //权重
  };
  typedef std::vector<InvertedElem> InvertedList;//倒排拉链

  class Index
  {
    private:
      //正排索引的数据结构用数组，数组下标天然对应文档ID
      std::vector<DocInfo> forward_index;

      //倒排索引一定是一个关键字和一组（个）InvertedElem 对应
      //就是   关键字和倒排拉链 相映射
      std::unordered_map<std::string,InvertedList> inverted_index;//倒排索引
    private:
      Index(){};//一定要有函数体，不能delete
      Index(const Index&) = delete;
      Index& operator=(const Index&) = delete;
      
      static Index* instance;
      static std::mutex mtx;
    public:
      ~Index();
    public:

      //获取单例函数
      static Index* GetInstance()
      {
        if(nullptr == instance)
        {
          mtx.lock();
          if(nullptr == instance)
          {
            
            instance = new Index();

          }
          mtx.unlock();
        }
        return instance;
      }

      //根据doc_id找到文档内容
      DocInfo* GetForwardIndex(uint64_t doc_id)
      {
        if(doc_id >= forward_index.size())
        {
          std::cerr << " doc_id out range,error! " << std::endl;
          return nullptr;
        }
        return &forward_index[doc_id];
      }

      //根据关键字string，获得倒排拉链
      InvertedList* GetInvertedList(const std::string& word)
      {
        auto iter = inverted_index.find(word);
        if(iter == inverted_index.end())
        {
          std::cerr << word << " have no InvertedList! " << std::endl;
          return nullptr;
        }
        return &(iter->second);
      }

      //根据去标签，格式化之后的文档，构建正排和倒排索引  //data/raw_html/raw.txt
      bool BuildIndex(const std::string& input)//parser处理完成的数据
      {
        //处理好的数据文件打开，按行读取
        std::ifstream in(input,std::ios::in | std::ios::binary);
        if(!in.is_open())
        {
          std::cerr << "Sorry," << input << "open error" <<std::endl;
          return false;
        }
        
        //现在文件已经打开
        std::string line;
        while(std::getline(in,line))
        {
          //开始循环读取
          //建立正排索引
          DocInfo* doc = BuildForwardIndex(line);
          if(nullptr == doc)
          {
            std::cerr << "Build" << line << "error" << std::endl;
            continue;
          }
          
          //建立倒排索引
          BuildInvertedIndex(*doc);
        }
        return true;
      }




    private:
      DocInfo* BuildForwardIndex(const std::string& line)
      {
        //1.解析line,字符串切分 line-> 3个string (title,content,url)
        std::vector<std::string> results;
        const std::string sep = "\3";

        ns_util::StringUtil::Split(line,&results,sep);
        if(results.size() != 3)
        {
          return nullptr;
        }
        
        //2.字符串进行填充到DocInfo
        DocInfo doc;
        doc.title = results[0];
        doc.content = results[1];
        doc.url = results[2];
        doc.doc_id = forward_index.size();//先进行保存ID在插入，对应ID就是当前doc在vector中的下标

        //3.插入到正排索引的vector
        forward_index.push_back(std::move(doc));
        return &forward_index.back();
      }



      bool BuildInvertedIndex(const DocInfo& doc)
      {
        //DocInfo(title,content,url,doc_id)
        //word->倒排拉链
        struct word_cnt
        {
          int title_cnt;
          int content_cnt;
          word_cnt()
            :title_cnt(0)
             ,content_cnt(0)
          {}
        };

        //建立一个map来暂存词频的映射表
        std::unordered_map<std::string,word_cnt> word_map;

        //对标题进行分词
        std::vector<std::string> title_words;
        ns_util::JiebaUtil::CutString(doc.title,&title_words);
        //对标题进行词频统计
        for(std::string s : title_words)
        {
          boost::to_lower(s);//需要统一转换成小写
          word_map[s].title_cnt++;//如果存在就获取，反正则新建
        }


        //对内容进行分词
        std::vector<std::string> content_words;
        ns_util::JiebaUtil::CutString(doc.content,&content_words);
        //对内容进行词频统计
        for(std::string s : content_words)
        {
          boost::to_lower(s);
          word_map[s].content_cnt++;
        }


#define x 10
#define y 1

        //遍历映射表
        for(auto& words_pair : word_map)
        {
          InvertedElem item;//一个元素
          item.doc_id = doc.doc_id;
          item.word = words_pair.first;
          item.weight = x*words_pair.second.title_cnt + y*words_pair.second.content_cnt;//相关性

          //更新到倒排索引中
          InvertedList& inverted_list = inverted_index[words_pair.first];

          inverted_list.push_back(std::move(item));
        }
        return true;
      }
  };
  Index* Index::instance = nullptr;
  std::mutex Index::mtx;
}
