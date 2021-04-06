#ifndef CPPJIEBA_DICT_TRIE_HPP
#define CPPJIEBA_DICT_TRIE_HPP

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <cmath>
#include <limits>
#include "limonp/StringUtil.hpp"
#include "limonp/Logging.hpp"
#include "Unicode.hpp"
#include "Trie.hpp"

namespace cppjieba {

using namespace limonp;

const double MIN_DOUBLE = -3.14e+100;
const double MAX_DOUBLE = 3.14e+100;
const size_t DICT_COLUMN_NUM = 3;
const char* const UNKNOWN_TAG = "";

#define CACHE_FILE         "Cache_File.mc"
#define CACHE_DATA_MAX_LEN 1024

class DictTrie {
 public:
  enum UserWordWeightOption {
    WordWeightMin,
    WordWeightMedian,
    WordWeightMax,
  }; // enum UserWordWeightOption

  DictTrie(const string& dict_path, const string& user_dict_paths = "", UserWordWeightOption user_word_weight_opt = WordWeightMedian) {
    Init(dict_path, user_dict_paths, user_word_weight_opt);
  }

  ~DictTrie() {
    delete trie_;
  }

  bool InsertUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    if (!MakeNodeInfo(node_info, word, user_word_default_weight_, tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  bool InsertUserWord(const string& word,int freq, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    double weight = freq ? log(1.0 * freq / freq_sum_) : user_word_default_weight_ ;
    if (!MakeNodeInfo(node_info, word, weight , tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  const DictUnit* Find(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end) const {
    return trie_->Find(begin, end);
  }

  void Find(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        vector<struct Dag>&res,
        size_t max_word_len = MAX_WORD_LENGTH) const {
    trie_->Find(begin, end, res, max_word_len);
  }

  bool Find(const string& word)
  {
    const DictUnit *tmp = NULL;
    RuneStrArray runes;
    if (!DecodeRunesInString(word, runes))
    {
      XLOG(ERROR) << "Decode failed.";
    }
    tmp = Find(runes.begin(), runes.end());
    if (tmp == NULL)
    {
      return false;
    }
    else
    {
      return true;
    }
  }

  bool IsUserDictSingleChineseWord(const Rune& word) const {
    return IsIn(user_dict_single_chinese_word_, word);
  }

  double GetMinWeight() const {
    return min_weight_;
  }

  void InserUserDictNode(const string& line) {
    vector<string> buf;
    DictUnit node_info;
    Split(line, buf, " ");
    if(buf.size() == 1){
          MakeNodeInfo(node_info, 
                buf[0], 
                user_word_default_weight_,
                UNKNOWN_TAG);
        } else if (buf.size() == 2) {
          MakeNodeInfo(node_info, 
                buf[0], 
                user_word_default_weight_,
                buf[1]);
        } else if (buf.size() == 3) {
          int freq = atoi(buf[1].c_str());
          assert(freq_sum_ > 0.0);
          double weight = log(1.0 * freq / freq_sum_);
          MakeNodeInfo(node_info, buf[0], weight, buf[2]);
        }
        static_node_infos_.push_back(node_info);
        if (node_info.word.size() == 1) {
          user_dict_single_chinese_word_.insert(node_info.word[0]);
        }
  }
  
  void LoadUserDict(const vector<string>& buf) {
    for (size_t i = 0; i < buf.size(); i++) {
      InserUserDictNode(buf[i]);
    }
  }

   void LoadUserDict(const set<string>& buf) {
    std::set<string>::const_iterator iter;
    for (iter = buf.begin(); iter != buf.end(); iter++){
      InserUserDictNode(*iter);
    }
  }

  void LoadUserDict(const string& filePaths) {
    vector<string> files = limonp::Split(filePaths, "|;");
    size_t lineno = 0;
    for (size_t i = 0; i < files.size(); i++) {
      ifstream ifs(files[i].c_str());
      XCHECK(ifs.is_open()) << "open " << files[i] << " failed"; 
      string line;
      
      for (; getline(ifs, line); lineno++) {
        if (line.size() == 0) {
          continue;
        }
        InserUserDictNode(line);
      }
    }
  }


 private:
  void Init(const string& dict_path, const string& user_dict_paths, UserWordWeightOption user_word_weight_opt) {
	if(UnSerialization() == false)
	{
		LoadDict(dict_path);
		freq_sum_ = CalcFreqSum(static_node_infos_);
		CalculateWeight(static_node_infos_, freq_sum_);
		SetStaticWordWeights(user_word_weight_opt);

		if (user_dict_paths.size()) {
		  LoadUserDict(user_dict_paths);
		}
		Shrink(static_node_infos_);

		Serialization();
	}

    CreateTrie(static_node_infos_);
  }
  
  void CreateTrie(const vector<DictUnit>& dictUnits) {
    assert(dictUnits.size());
    vector<Unicode> words;
    vector<const DictUnit*> valuePointers;
    for (size_t i = 0 ; i < dictUnits.size(); i ++) {
      words.push_back(dictUnits[i].word);
      valuePointers.push_back(&dictUnits[i]);
    }

    trie_ = new Trie(words, valuePointers);
  }

  


  bool MakeNodeInfo(DictUnit& node_info,
        const string& word, 
        double weight, 
        const string& tag) {
    if (!DecodeRunesInString(word, node_info.word)) {
      XLOG(ERROR) << "Decode " << word << " failed.";
      return false;
    }
    node_info.weight = weight;
    node_info.tag = tag;
    return true;
  }

  void LoadDict(const string& filePath) {
    ifstream ifs(filePath.c_str());
    XCHECK(ifs.is_open()) << "open " << filePath << " failed.";
    string line;
    vector<string> buf;

    DictUnit node_info;
    for (size_t lineno = 0; getline(ifs, line); lineno++) {
      Split(line, buf, " ");
      XCHECK(buf.size() == DICT_COLUMN_NUM) << "split result illegal, line:" << line;
      MakeNodeInfo(node_info, 
            buf[0], 
            atof(buf[1].c_str()), 
            buf[2]);
      static_node_infos_.push_back(node_info);
    }
  }

  static bool WeightCompare(const DictUnit& lhs, const DictUnit& rhs) {
    return lhs.weight < rhs.weight;
  }

  void SetStaticWordWeights(UserWordWeightOption option) {
    XCHECK(!static_node_infos_.empty());
    vector<DictUnit> x = static_node_infos_;
    sort(x.begin(), x.end(), WeightCompare);
    min_weight_ = x[0].weight;
    max_weight_ = x[x.size() - 1].weight;
    median_weight_ = x[x.size() / 2].weight;
    switch (option) {
     case WordWeightMin:
       user_word_default_weight_ = min_weight_;
       break;
     case WordWeightMedian:
       user_word_default_weight_ = median_weight_;
       break;
     default:
       user_word_default_weight_ = max_weight_;
       break;
    }
  }

  double CalcFreqSum(const vector<DictUnit>& node_infos) const {
    double sum = 0.0;
    for (size_t i = 0; i < node_infos.size(); i++) {
      sum += node_infos[i].weight;
    }
    return sum;
  }

  void CalculateWeight(vector<DictUnit>& node_infos, double sum) const {
    assert(sum > 0.0);
    for (size_t i = 0; i < node_infos.size(); i++) {
      DictUnit& node_info = node_infos[i];
      assert(node_info.weight > 0.0);
      node_info.weight = log(double(node_info.weight)/sum);
    }
  }

  int SerializationDictUnit(char* data_temp_, size_t data_max_size_, DictUnit& _dict_unit)
  {
	  size_t pos_ = 0;

	  size_t word_size_ = _dict_unit.word.size();
	  if(data_max_size_ > pos_ + sizeof(int))
	  {
		memcpy(&data_temp_[pos_], (char *)&word_size_, sizeof(int));
		pos_ += sizeof(int);
	  }

	  cppjieba::Unicode unicode_ = _dict_unit.word;
	  for(size_t i = 0; i < word_size_; i++)
	  {
		  if(data_max_size_ > pos_ + sizeof(Rune))
		  {
			memcpy(&data_temp_[pos_], (char *)&unicode_[i], sizeof(Rune));
			pos_ += sizeof(Rune);
		  }
	  }

	  if(data_max_size_ > pos_ + sizeof(_dict_unit.weight))
	  {
		memcpy(&data_temp_[pos_], (char *)&_dict_unit.weight, sizeof(_dict_unit.weight));
		pos_ += sizeof(_dict_unit.weight);
	  }

	  short tag_len_ = _dict_unit.tag.length();
	  if(data_max_size_ > pos_ + sizeof(short) + tag_len_)
	  {
		memcpy(&data_temp_[pos_], (char *)&tag_len_, sizeof(short));
		pos_ += sizeof(short);
		memcpy(&data_temp_[pos_], (char *)_dict_unit.tag.c_str(), tag_len_);
		pos_ += tag_len_;
	  }

	  return (int)pos_;
  }

  bool Serialization()
  {
	  //序列化当前内存结构为文件，增加读效率
	  //因为vector是连续的，有一种简单的方法是直接将内存地址映射成文件，但是考虑到缓冲可移植的问题，保证在不同的机器上缓冲层可用
	  //所以使用了逐个变量序列化的方式。
	  int data_temp_max_len_ = CACHE_DATA_MAX_LEN;
	  char data_temp_[CACHE_DATA_MAX_LEN] = {'\0'};

	  FILE* cache_file_ = fopen(CACHE_FILE, "wb");
	  if(NULL == cache_file_)
	  {
		  XLOG(ERROR) << "file cache file create (" << CACHE_FILE << ") failed.";
		  return false;
	  }

	  //序列化基本元素
	  memcpy(data_temp_, (char* )&freq_sum_, sizeof(double));
	  fwrite(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy(data_temp_, (char* )&min_weight_, sizeof(double));
	  fwrite(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy(data_temp_, (char* )&max_weight_, sizeof(double));
	  fwrite(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy(data_temp_, (char* )&median_weight_, sizeof(double));
	  fwrite(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy(data_temp_, (char* )&user_word_default_weight_, sizeof(double));
	  fwrite(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  fflush(cache_file_);

	  int info_size_ = static_node_infos_.size();
	  memcpy(data_temp_, (char* )&info_size_, sizeof(int));
	  fwrite(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  for(int i = 0; i < info_size_; i++)
	  {
		memset(data_temp_, 0, CACHE_DATA_MAX_LEN);
		int data_len_ = SerializationDictUnit(data_temp_, (size_t)data_temp_max_len_, static_node_infos_[i]);
		fwrite(data_temp_, sizeof(char), data_len_, cache_file_);
	  }
	  fflush(cache_file_);

	  int active_info_size_ = active_node_infos_.size();
	  memcpy(data_temp_, (char* )&active_info_size_, sizeof(int));
	  fwrite(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  for(int i = 0; i < active_info_size_; i++)
	  {
		 int data_len_ = SerializationDictUnit(data_temp_, (size_t)data_temp_max_len_, active_node_infos_[i]);
		 fwrite(data_temp_, sizeof(char), data_len_, cache_file_);
	  }
	  fflush(cache_file_);

	  int single_chinese_word_size_ = user_dict_single_chinese_word_.size();
	  memcpy(data_temp_, (char* )&single_chinese_word_size_, sizeof(int));
	  fwrite(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  for (unordered_set<Rune>::iterator p = user_dict_single_chinese_word_.begin(); p != user_dict_single_chinese_word_.end(); ++p)
	  {
		 Rune& rune_ = (Rune& )*p;
		 memcpy(data_temp_, (char* )&rune_, sizeof(Rune));
		 fwrite(data_temp_, sizeof(char), sizeof(Rune), cache_file_);
	  }
	  fflush(cache_file_);

	  fclose(cache_file_);
	  return true;
  }

  bool UnSerializationDictUnit(FILE* cache_file_, DictUnit& dict_unit_)
  {
	  int data_temp_max_len_ = CACHE_DATA_MAX_LEN;
	  char data_temp_[CACHE_DATA_MAX_LEN] = {'\0'};

	  size_t word_size_ = 0;
	  fread(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  memcpy((char* )&word_size_, data_temp_, sizeof(int));
	  for(size_t i = 0; i < word_size_; i++)
	  {
		  Rune rune_;
		  fread(data_temp_, sizeof(char), sizeof(Rune), cache_file_);
		  memcpy((char* )&rune_, data_temp_, sizeof(Rune));
		  dict_unit_.word.push_back(rune_);
	  }

	  fread(data_temp_, sizeof(char), sizeof(dict_unit_.weight), cache_file_);
	  memcpy((char* )&dict_unit_.weight, data_temp_, sizeof(dict_unit_.weight));

	  short tag_len_ = 0;
	  fread(data_temp_, sizeof(char), sizeof(short), cache_file_);
	  memcpy((char* )&tag_len_, data_temp_, sizeof(short));
	  if(tag_len_ < data_temp_max_len_)
	  {
		fread(data_temp_, sizeof(char), tag_len_, cache_file_);
		data_temp_[tag_len_] = '\0';
		dict_unit_.tag = (string)data_temp_;
	  }

	  return true;
  }

  bool UnSerialization()
  {
	  //反序列化
	  char data_temp_[CACHE_DATA_MAX_LEN] = {'\0'};

	  FILE* cache_file_ = fopen(CACHE_FILE, "rb");
	  if(NULL == cache_file_)
	  {
		  //XLOG(ERROR) << "file cache file read (" << CACHE_FILE << ") failed.";
		  return false;
	  }

	  fread(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy((char* )&freq_sum_, data_temp_, sizeof(double));
	  fread(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy((char* )&min_weight_, data_temp_, sizeof(double));
	  fread(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy((char* )&max_weight_, data_temp_, sizeof(double));
	  fread(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy((char* )&median_weight_, data_temp_, sizeof(double));
	  fread(data_temp_, sizeof(char), sizeof(double), cache_file_);
	  memcpy((char* )&user_word_default_weight_, data_temp_, sizeof(double));

	  int info_size_ = 0;
	  fread(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  memcpy((char* )&info_size_, data_temp_, sizeof(int));

	  
	  for(int i = 0; i < info_size_; i++)
	  {
		  DictUnit dict_unit_;
		  UnSerializationDictUnit(cache_file_, dict_unit_);
		  static_node_infos_.push_back(dict_unit_);
	  }

	  int active_info_size_ = 0;
	  fread(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  memcpy((char* )&active_info_size_, data_temp_, sizeof(int));
	  for(int i = 0; i < active_info_size_; i++)
	  {
		  DictUnit dict_unit_;
		  UnSerializationDictUnit(cache_file_, dict_unit_);
		  active_node_infos_.push_back(dict_unit_);
	  }

	  int single_chinese_word_size_ = 0;
	  fread(data_temp_, sizeof(char), sizeof(int), cache_file_);
	  memcpy((char* )&single_chinese_word_size_, data_temp_, sizeof(int));
	  for(int i = 0; i < single_chinese_word_size_; i++)
	  {
		  Rune rune_;
		  fread(data_temp_, sizeof(char), sizeof(Rune), cache_file_);
		  memcpy((char* )&rune_, data_temp_, sizeof(Rune));
		  user_dict_single_chinese_word_.insert(rune_);
	  }

	  fclose(cache_file_);

	  return true;
  }

  void Shrink(vector<DictUnit>& units) const {
    vector<DictUnit>(units.begin(), units.end()).swap(units);
  }

  vector<DictUnit> static_node_infos_;
  deque<DictUnit> active_node_infos_; // must not be vector
  Trie * trie_;

  double freq_sum_;
  double min_weight_;
  double max_weight_;
  double median_weight_;
  double user_word_default_weight_;
  unordered_set<Rune> user_dict_single_chinese_word_;
};
}

#endif
