#include<iostream>
#include<string>
#include<vector>
#include<boost/filesystem.hpp>
#include"util.hpp"


const std::string src_path = "data/input";
//是一个目录，里面放的是所有html网页
const std::string output = "data/raw_html/raw.txt";//写入

typedef struct DocInfo
{
  std::string title;//文档标题
  std::string content;//文档内容
  std::string url;//文档官网中的url
}DocInfo_t;

//声明函数
bool EnumFile(const std::string& src_path,std::vector<std::string>* file_list);
bool ParseHtml(const std::vector<std::string>& file_list,std::vector<DocInfo>* results);
bool SaveHtml(const std::vector<DocInfo>& results,const std::string& output);

int main()
{
  //第一步：递归式的把每个html文件名+路径，保存到file_list中，方便后期一个一个的进行读取
  std::vector<std::string> file_list;
  if(!EnumFile(src_path,&file_list))
  {
    std::cerr << " enum file name error " << std::endl;
    return 1;
  }

  //第二步：按照file_list读取每个文件内容，并进行解析
  std::vector<DocInfo_t> results;
  if(!ParseHtml(file_list,&results))
  {
    std::cerr << " parse html error " << std::endl;
    return 2;
  }

  //第三步：把解析完成的每个文件写入到output中，按\3为分隔符
  if(!SaveHtml(results,output))
  {
    std::cerr<< " save html error " << std::endl;
    return 3;
  }
  return 0;
}

bool EnumFile(const std::string& src_path,std::vector<std::string>* file_list)
{
  namespace fs = boost::filesystem;//指明作用域
  fs::path root_path(src_path);//用库中函数path定义对象
  
  //判断路径是否存在
  if(!fs::exists(root_path))
  {
    std::cerr << src_path << " not exists " << std::endl;
    return false;
  }

  //定义一个空的迭代器，用来表示是否递归结束
  fs::recursive_directory_iterator end;
  for(fs::recursive_directory_iterator iter(root_path); iter != end; ++iter)
  {
    //判断文件是否是普通文件，html都是普通文件
    if(!fs::is_regular_file(*iter))
    {
      continue;
    }
    //判断文件后缀是否是.html
    if(iter->path().extension() != ".html")
    {
      continue;
    }
    

    
    //当前路径是合法的，符合条件，保存到file_list中
    file_list->push_back(iter->path().string());
  }
  return true;
}



//测：
static void ShowDoc(const DocInfo& doc)
{
  std::cout << "title-- : " << doc.title << std::endl;
  std::cout << "content-- : " << doc.content << std::endl;
  std::cout << "url-- : " << doc.url << std::endl;
}
static bool ParseTitle(const std::string& file,std::string* title)
{
  std::size_t begin = file.find("<title>");
  if(begin == std::string::npos)
  {
    return false;
  }
  std::size_t end = file.find("</title>");
  if(end == std::string::npos)
  {
    return false;
  }
  begin += std::string("<title>").size();
  if(begin > end)
  {
    return false;
  }
  *title = file.substr(begin,end-begin);
  return true;
}
static bool ParseContent(const std::string& file,std::string* content)
{
  //去标签，基于一个简易状态机
  enum status
  {
    LABLE,
    CONTENT
  };
  enum status s = LABLE;
  for(char c : file)
  {
    switch(s)
    {
      case LABLE:
        if(c == '>')
        {
          s = CONTENT;
        }
        break;
      case CONTENT:
        if(c == '<')
        {
          s = LABLE;
        }
        else 
        {
          if(c == '\n')
          {
            c = ' ';
          }
          content->push_back(c);
        }
        break;
      default:
        break;
    }
  }
  return true;
}
static bool ParseUrl(const std::string &file_path, std::string *url)
{
    std::string url_head = "https://www.boost.org/doc/libs/1_79_0/doc/html";
    std::string url_tail = file_path.substr(src_path.size());

    *url = url_head + url_tail;
    return true;
}



bool ParseHtml(const std::vector<std::string>& file_list,std::vector<DocInfo>* results)
{
  //遍历
  for(const std::string& file : file_list)
  {
    //1.读取文件，Read()
    std::string result;//读到文件放入

    if(!ns_util::FileUtil::ReadFile(file,&result))
    {
      continue;
    }

    DocInfo_t doc;
    //2.解析指定文件路径，提取title
    if(!ParseTitle(result,&doc.title))
    {
      continue;
    }
    //3.解析指定文件，提取content，就是去标签
    if(!ParseContent(result,&doc.content))
    {
      continue;
    }
    //4.解析指定文件路径，构建URL
    if(!ParseUrl(file,&doc.url))
    {
      continue;
    }

    //完成解析，都保存在doc中
    results->push_back(std::move(doc));
    
    //测：
    //ShowDoc(doc);
    //break;
  }
  return true;
}
bool SaveHtml(const std::vector<DocInfo>& results,const std::string& output)
{
  #define SEP '\3'
    //按照二进制方式进行写入
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if(!out.is_open()){
        std::cerr << "open " << output << " failed!" << std::endl;
        return false;
    }

    //就可以进行文件内容的写入了
    for(auto& item : results){
        std::string out_string;
        out_string = item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';

        out.write(out_string.c_str(), out_string.size());
    }

    out.close();
  return true;
}

