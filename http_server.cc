#include"searcher.hpp"

#include "cpp-httplib/httplib.h"
#include"log.hpp"

const std::string input = "data/raw_html/raw.txt";
const std::string root_path = "./wwwroot";

int main()
{
  //定义对象：
  ns_searcher::Searcher search;

  //搜索时建库对应的数据源
  search.InitSearcher(input);

  httplib::Server svr;
  svr.set_base_dir(root_path.c_str());//默认网页

  svr.Get("/s",[&search](const httplib::Request& req,httplib::Response& rsp)
      {
        //判断是否有参数
        if(!req.has_param("word"))
        {
          rsp.set_content("必须要有搜索关键字！","text/plain; charset=utf-8");
          return;
        }
        //获取用户输入的参数
        std::string word = req.get_param_value("word");
        //std::cout << "用户在搜索: " << word << std::endl;
        LOG(NORMAL," 用户在搜索: " + word);

        std::string json_string;
        search.Search(word,&json_string);
        //返回给用户
        rsp.set_content(json_string,"application/json");
      });

  LOG(NORMAL," 服务器启动成功！---> ");
  svr.listen("0.0.0.0",8888);
  return 0;
}
