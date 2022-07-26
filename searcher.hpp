#pragma once

#include"index.hpp"
#include"util.hpp"
#include<unordered_map>
#include<jsoncpp/json/json.h>
#include <algorithm>

#include"log.hpp"

namespace ns_searcher
{

  //自己手动定义一个倒排拉链的节点来显示
  struct InvertedElemPrint
  {
    uint16_t doc_id;
    int weight;
    //多个词放到一起的vector
    std::vector<std::string> words;

    InvertedElemPrint()
      :doc_id(0)
      ,weight(0)
      {}
  };
  class Searcher
  {
  private:
    ns_index::Index *index; //供系统进行查找的索引
  public:
    Searcher() {}
    ~Searcher() {}

  public:
    void InitSearcher(const std::string &input)
    {
      // 1.获取或创建单例index对象
      index = ns_index::Index::GetInstance();
      //std::cout << " 获取index单例成功！... " << std::endl;
      LOG(NORMAL," 获取index单例成功!---> ");
      // 2.根据index对象，建立索引
      index->BuildIndex(input);
      //std::cout << " 建立正排和倒排索引成功！... " << std::endl;
      LOG(NORMAL," 建立正排和倒排索引成功!---> ");
    }

    void Search(const std::string &query, std::string *json_string)
    {
      // 1.分词：对我们的query进行按照searcher的要求进行分词
      std::vector<std::string> words;
      ns_util::JiebaUtil::CutString(query, &words);
      // 2.触发：根据分词的各个词，进行index查找
      //ns_index::InvertedList inverted_list_all; //按照InvertedElem

      std::vector<InvertedElemPrint> inverted_list_all;

      std::unordered_map<uint64_t,InvertedElemPrint> tokens_map;//后面可以根据doc_id去重


      for (std::string word : words)
      {
        boost::to_lower(word);
        ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
        if (nullptr == inverted_list)
        {
          continue;
        }
        //inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());

        //将获得的所有倒排拉链节点遍历
        for(const auto& elem : *inverted_list)
        {
          auto& item = tokens_map[elem.doc_id];
          //item一定是doc_id相同的InvertedElemPrint节点
          item.doc_id = elem.doc_id;
          item.weight += elem.weight;
          item.words.push_back(elem.word);
        }

        for(const auto& item : tokens_map)
        {
          inverted_list_all.push_back(std::move(item.second));
        }
      }
      // 3.合并排序：汇总查找结果，按照相关性降序排序
      // std::sort(inverted_list_all.begin(), inverted_list_all.end(),
      //           [](const ns_index::InvertedElem &e1, const ns_index::InvertedElem &e2)
      //           {
      //             return e1.weight > e2.weight;
      //           });


      std::sort(inverted_list_all.begin(), inverted_list_all.end(),
                [](const InvertedElemPrint& e1,const InvertedElemPrint& e2)
                {
                  return e1.weight > e2.weight;
                });
      // 4.构建：根据查找出来的结果，构建json串--jsoncpp
      Json::Value root;
      for (auto &item : inverted_list_all)
      {
        ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
        if (nullptr == doc)
        {
          continue;
        }
        Json::Value elem;
        elem["title"] = doc->title;
        elem["dest"] = GetDest(doc->content, item.words[0]); //可是我们只想要一部分
        elem["url"] = doc->url;

        //for deubg for delete
        // elem["id"] = (int)item.doc_id;
        // elem["weight"] = item.weight;
        
        root.append(elem);
      }
      Json::StyledWriter writer;
      *json_string = writer.write(root);
    }

    std::string GetDest(const std::string &html_content, const std::string &word)
    {
      //找到word在html content中首次出现，然后往前找50个字节，往后找100个字节。
      const int prev_step = 50;
      const int next_step = 100;
      // 1->找到首次出现
      //不能使用find来查找

      auto iter = std::search(html_content.begin(), html_content.end(), word.begin(), word.end(),
                  [](int x, int y)
                  { 
                    return (std::tolower(x) == std::tolower(y)); 
                  });
      if (iter == html_content.end())
      {
        return "None1";
      }
      int pos = std::distance(html_content.begin(), iter); // pos 和 文档最开始的距离
      // 2.->获取start、end
      int start = 0;
      int end = html_content.size() - 1;
      //如果之前有50个字符就更新，之后有100个字符也更新
      if (pos > start + prev_step)
      {
        start = pos - prev_step;
      }
      if (pos < end - next_step)
      {
        end = pos + next_step;
      }

      // 3.->截取子串
      if (start > end)
      {
        return "None2";
      }
      std::string desc = html_content.substr(start, end - start);
      desc += " ...! ";
      return desc;
    }
  };
}
