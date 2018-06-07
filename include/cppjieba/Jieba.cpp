#ifndef CPPJIEAB_JIEBA_H
#define CPPJIEAB_JIEBA_H

#include "QuerySegment.hpp"
#include "KeywordExtractor.hpp"
using namespace std;


namespace cppjieba {

class Jieba {
 public:
  Jieba(const string& dict_path, 
        const string& model_path,
        const string& user_dict_path, 
        const string& idfPath, 
        const string& stopWordPath) 
    : dict_trie_(dict_path, user_dict_path),
      model_(model_path),
      mp_seg_(&dict_trie_),
      hmm_seg_(&model_),
      mix_seg_(&dict_trie_, &model_),
      full_seg_(&dict_trie_),
      query_seg_(&dict_trie_, &model_),
      extractor(&dict_trie_, &model_, idfPath, stopWordPath) {
  }
  ~Jieba() {
  }

  struct LocWord {
    string word;
    size_t begin;
    size_t end;
  }; // struct LocWord

  void Cut(const string& sentence, vector<string>& words, bool hmm = true) const {
    mix_seg_.Cut(sentence, words, hmm);
  }
  void Cut(const string& sentence, vector<Word>& words, bool hmm = true) const {
    mix_seg_.Cut(sentence, words, hmm);
  }
  void CutAll(const string& sentence, vector<string>& words) const {
    full_seg_.Cut(sentence, words);
  }
  void CutAll(const string& sentence, vector<Word>& words) const {
    full_seg_.Cut(sentence, words);
  }
  void CutForSearch(const string& sentence, vector<string>& words, bool hmm = true) const {
    query_seg_.Cut(sentence, words, hmm);
  }
  void CutForSearch(const string& sentence, vector<Word>& words, bool hmm = true) const {
    query_seg_.Cut(sentence, words, hmm);
  }
  void CutHMM(const string& sentence, vector<string>& words) const {
    hmm_seg_.Cut(sentence, words);
  }
  void CutHMM(const string& sentence, vector<Word>& words) const {
    hmm_seg_.Cut(sentence, words);
  }
  void CutSmall(const string& sentence, vector<string>& words, size_t max_word_len) const {
    mp_seg_.Cut(sentence, words, max_word_len);
  }
  void CutSmall(const string& sentence, vector<Word>& words, size_t max_word_len) const {
    mp_seg_.Cut(sentence, words, max_word_len);
  }
  
  void Tag(const string& sentence, vector<pair<string, string> >& words) const {
    mix_seg_.Tag(sentence, words);
  }
  string LookupTag(const string &str) const {
    return mix_seg_.LookupTag(str);
  }
  bool InsertUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    return dict_trie_.InsertUserWord(word, tag);
  }

  void ResetSeparators(const string& s) {
    //TODO
    mp_seg_.ResetSeparators(s);
    hmm_seg_.ResetSeparators(s);
    mix_seg_.ResetSeparators(s);
    full_seg_.ResetSeparators(s);
    query_seg_.ResetSeparators(s);
  }

  const DictTrie* GetDictTrie() const {
    return &dict_trie_;
  } 
  const HMMModel* GetHMMModel() const {
    return &model_;
  }

 private:
  DictTrie dict_trie_;
  HMMModel model_;
  
  // They share the same dict trie and model
  MPSegment mp_seg_;
  HMMSegment hmm_seg_;
  MixSegment mix_seg_;
  FullSegment full_seg_;
  QuerySegment query_seg_;

 public:
  KeywordExtractor extractor;
}; // class Jieba

} // namespace cppjieba

#endif // CPPJIEAB_JIEBA_H

using namespace cppjieba;
char* DICT_PATH = "dict/jieba.dict.utf8";
char* HMM_PATH = "dict/hmm_model.utf8";
char* USER_DICT_PATH = "dict/user.dict.utf8";
char* IDF_PATH = "dict/idf.utf8";
char* STOP_WORD_PATH = "dict/stop_words.utf8";

#include <python2.7/Python.h>

extern "C" {
    /*需要以下接口:
    seg.__init__()
    .add_word(word,num,tag)
    .del_word(word)
    .all_cut(sentence)
    .cut(sentence)
    .tag(sentence)
    key_words()
    */
    Jieba* Jieba_new(char* dict_path, 
        char* model_path,
        char* user_dict_path, 
        char* idfPath, 
        char* stopWordPath){ 
            return new Jieba(dict_path, model_path, user_dict_path, idfPath, stopWordPath);}
    
    PyObject* Jieba_cut(Jieba* segmentor, char* sentence, bool hmm = true){
        PyObject* result = PyList_New(0);
        vector<string> words;
        segmentor->Cut(sentence, words, hmm);
        for (vector<string>::const_iterator iter =words.begin(); iter != words.end(); iter++){
            PyObject* a=PyString_FromString((*iter).c_str());
            PyList_Append(result,a);
            Py_XDECREF(a);
        }
        //free(words);
        return result;
    }

    PyObject* Jieba_tag(Jieba *segmentor, char* sentence){
        PyObject* result = PyList_New(0);
        vector<pair<string, string> > tagers;
        segmentor->Tag(sentence, tagers);
        for (vector<pair<string, string> >::const_iterator iter =tagers.begin(); iter != tagers.end(); iter++){
            /*
            PyObject* a=PyString_FromString((iter->first).c_str());
            PyObject* b=PyString_FromString((iter->second).c_str());
            PyObject* p=PyTuple_Pack(2,a,b);//很奇怪，有时候core dumped...
            */
            string s=iter->first+'/'+iter->second;
            PyObject* p=PyString_FromString(s.c_str());
            PyList_Append(result,p);
            Py_XDECREF(p);
        }
        return result;
    }

    PyObject* Jieba_extract(Jieba *segmentor, char* sentence, int topN){
        PyObject* result = PyList_New(0);
        vector<pair<string, double> > keywords;
        //vector<Word> keywords;
        segmentor->extractor.Extract(sentence, keywords, topN);
        for (vector< pair<string, double> >::const_iterator iter =keywords.begin(); iter != keywords.end(); iter++){
            //cout << *iter <<endl;
            //string s=(*iter).word+'/'+(*iter).weight.c_str();
            PyObject* weight=PyFloat_FromDouble(iter->second);
            PyObject* word=PyString_FromString(iter->first.c_str());
            PyObject* p=PyTuple_Pack(2,word,weight);
            PyList_Append(result,p);
            Py_XDECREF(weight);
            Py_XDECREF(word);
            Py_XDECREF(p);

        }
        return result;
    }

}

int main(int argc, char** argv) {
    Jieba* segmentor=Jieba_new(DICT_PATH,
        HMM_PATH,
        USER_DICT_PATH,
        IDF_PATH,
        STOP_WORD_PATH); 
    char* s = "他来到了网易杭研大厦";
    s = "全国两会是数千名中外记者关注中国发展、聚焦中国命运的“新闻发布厅”。“本届两会，处处彰显着‘开门办会’的会风，大家走进了更开放的会场，体会到了更民主的氛围。”光明日报社融媒体记者张永群说。    　　“开放日”不避敏感话题    　　3月7日上午9时，十二届全国人大五次会议宁夏代表团举行团组开放会议，全程向境内外媒体开放。32家境内媒体、13家境外媒体的70多名记者提前入场，抢占“有利地形”。    　　“自治区党委书记李建华回答了‘清真泛化’的问题，不回避敏感话题。”宁夏日报社记者李东梅说，“代表们坦率面对媒体，从容‘接招儿’。”    　　“这两天，我参加了贵州、宁夏、浙江等代表团的媒体‘开放日’，从务实的提案、建议到接地气的回答，可以说是干货满满。这种开放和坦诚的背后，正是治国理政新理念带来的积极变化。两会为媒体提供了一场新闻盛筵，也为世界提供了一扇了解中国的窗户。”张永群说。    　　今年头次跑两会的北京青年报记者邹春霞，则用“没想到”表达了她对政协新闻服务组的点赞，“没想到新闻服务这么周到，提供了各项会议的公开资料和安排，还提供了分组名单，每组的会议简报也很及时。新闻局建的记者服务微信群也非常管用，工作人员对我们的疑问和意见都能及时回复反馈。”    　　新华日报新媒体记者黄伟认为，今年全国两会新闻报道的特点是“快”，各家媒体都可以借助移动终端在网上平台直播，而且直播的题材也越来越多、时空越来越广，比如“部长通道”、代表委员访谈，体现了“移动优先、深度融合”的新趋势。    　　“部长通道”有问必答    　　本届两会，被追捧多届的“部长通道”成了外媒记者观察中国政府运行模式和思路的一个绝佳窗口。“记者问什么，部长们就答什么。”美国美中时报社副社长陆昱用“实实在在”四个字评价今年“部长通道”上的部长表现。第二年参加“部长通道”采访的陆昱认为，今年的“部长通道”进一步拉近了各部委与媒体之间的距离，这种面对面的交流互动使政策更透明，“也将在一定程度上拉近民心。”    　　“信息量很大，部长们回答得很积极很坦诚。”第二十一年参与全国两会报道的日本亚洲通讯社社长徐静波，今年是第四年在“部长通道”和众多国内外记者“抢新闻”。他发现，不少部长有备而来，不仅接受采访的态度十分积极，而且还有意识地借助这个平台介绍自己所属部门的工作开展情况以及对两会精神的理解。“‘部长通道’的开放，体现出中国执政的民主，也有效地建立起部长与媒体、部长与民众之间直接对话的渠道，意义非常重大。”    　　俄罗斯阿尔法电视台记者娜塔莎将今年“部长通道”的采访，形容为记者与部长之间“一次非常亲密的问答”。作为一名已采访全国两会10余年的海外记者，娜塔莎目睹了“部长通道”的变迁。她认为，今年“部长通道”最大的特点就是搭起了一个媒体记者与部长们平等对话的平台。娜塔莎感叹，这种和谐活跃的氛围、平等对话的形式折射出中国政府执政亲民化的趋势，也展现出一种随和的大国形象。    　　“开门办会”传递中国声音    　　“两会是了解中国发展的一扇窗户，政治、经济、民生，方方面面的话题都会在这里得到讨论和解答。”来自中非新闻交流中心的南苏丹记者阿汤加是第一次报道中国两会，他告诉记者，他最关心的是中国两会中会释放哪些与非洲政策、对非洲援助等有关的信息。他还非常关心中国的政治制度是如何运行的，“中国给非洲国家的发展提供了非常有价值的借鉴。”    　　俄罗斯塔斯社记者阿列克谢·谢里谢夫是一名资深驻华记者，在今年的两会外长记者会上，他就中俄美三国关系向外长提问。阿列克谢用流利的中文向记者感慨，“我感觉到中国正在变得更加自信和开放。我非常看好中国的稳定发展。”阿列克谢说。    　　日本朝日电视台的山本志门是第三次报道两会，他笑着告诉记者，报道中国两会是个“苦差事”。在短短十几天里，召开密集的记者会、出台各种新的政策，这样参与人数之多、影响之广的政治盛会，对于外国记者的报道来说是很大的考验。在今年的两会外长记者会上，山本提了有关“中日关系”的问题。他告诉本报记者，日本国内有非常多关于中国的报道，日本民众也很关心有关中国的各种话题。他希望日中两国能够加强沟通和了解。（本报记者 刘峰 杜一菲）";
    PyObject* result=Jieba_cut(segmentor, s);
    cout << s <<endl;
    result=Jieba_tag(segmentor, s);
    cout << s <<endl;
    result=Jieba_extract(segmentor, s, 10);
} 
