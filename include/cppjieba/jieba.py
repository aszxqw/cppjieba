#encoding=utf-8
from ctypes import *
import os
cur_dir = os.path.dirname( os.path.abspath(__file__)) or os.getcwd()

lib = cdll.LoadLibrary(cur_dir+'/libJieba.so')
lib.Jieba_cut.restype = py_object
lib.Jieba_tag.restype = py_object
lib.Jieba_extract.restype = py_object

class Tokenizer(object):
    def __init__(self, 
        dict_path=cur_dir+'/dict/jieba.dict.utf8', 
        model_path=cur_dir+'/dict/hmm_model.utf8', 
        user_dict_path=cur_dir+'/dict/user.dict.utf8', 
        idfPath=cur_dir+'/dict/idf.utf8', 
        stopWordPath=cur_dir+'/dict/stop_words.utf8'):
        self.obj = lib.Jieba_new(dict_path, model_path, user_dict_path, idfPath, stopWordPath)

    def add_word(self, word, tag='n', num=50):
        return lib.Jieba_add_word(self.obj, word, tag, num)
    
    def load_user_dicts(self, paths):
        return lib.Jieba_load_dict(self.obj, paths)

    def cut(self, sentence, hmm=True):
        return lib.Jieba_cut(self.obj, sentence, hmm)

    def pos_cut(self, sentence):
        rval=[]
        r=lib.Jieba_tag(self.obj, sentence)
        for s in r:
            a=s.split('/')
            rval.append(('/'.join(a[:-1]),a[-1]))
        return rval

    def extract(self, sentence, topN):
        return lib.Jieba_extract(self.obj, sentence, topN)

if __name__=='__main__':
    model=Tokenizer()
    print model
    text='''
    全国两会是数千名中外记者关注中国发展、聚焦中国命运的“新闻发布厅”。“本届两会，处处彰显着‘开门办会’的会风，大家走进了更开放的会场，体会到了更民主的氛围。”光明日报社融媒体记者张永群说。

        　　“开放日”不避敏感话题

            　　3月7日上午9时，十二届全国人大五次会议宁夏代表团举行团组开放会议，全程向境内外媒体开放。32家境内媒体、13家境外媒体的70多名记者提前入场，抢占“有利地形”。

                　　“自治区党委书记李建华回答了‘清真泛化’的问题，不回避敏感话题。”宁夏日报社记者李东梅说，“代表们坦率面对媒体，从容‘接招儿’。”

                    　　“这两天，我参加了贵州、宁夏、浙江等代表团的媒体‘开放日’，从务实的提案、建议到接地气的回答，可以说是干货满满。这种开放和坦诚的背后，正是治国理政新理念带来的积极变化。两会为媒体提供了一场新闻盛筵，也为世界提供了一扇了解中国的窗户。”张永群说。

                        　　今年头次跑两会的北京青年报记者邹春霞，则用“没想到”表达了她对政协新闻服务组的点赞，“没想到新闻服务这么周到，提供了各项会议的公开资料和安排，还提供了分组名单，每组的会议简报也很及时。新闻局建的记者服务微信群也非常管用，工作人员对我们的疑问和意见都能及时回复反馈。”

                            　　新华日报新媒体记者黄伟认为，今年全国两会新闻报道的特点是“快”，各家媒体都可以借助移动终端在网上平台直播，而且直播的题材也越来越多、时空越来越广，比如“部长通道”、代表委员访谈，体现了“移动优先、深度融合”的新趋势。

                                　　“部长通道”有问必答

                                    　　本届两会，被追捧多届的“部长通道”成了外媒记者观察中国政府运行模式和思路的一个绝佳窗口。“记者问什么，部长们就答什么。”美国美中时报社副社长陆昱用“实实在在”四个字评价今年“部长通道”上的部长表现。第二年参加“部长通道”采访的陆昱认为，今年的“部长通道”进一步拉近了各部委与媒体之间的距离，这种面对面的交流互动使政策更透明，“也将在一定程度上拉近民心。”

                                        　　“信息量很大，部长们回答得很积极很坦诚。”第二十一年参与全国两会报道的日本亚洲通讯社社长徐静波，今年是第四年在“部长通道”和众多国内外记者“抢新闻”。他发现，不少部长有备而来，不仅接受采访的态度十分积极，而且还有意识地借助这个平台介绍自己所属部门的工作开展情况以及对两会精神的理解。“‘部长通道’的开放，体现出中国执政的民主，也有效地建立起部长与媒体、部长与民众之间直接对话的渠道，意义非常重大。”

                                            　　俄罗斯阿尔法电视台记者娜塔莎将今年“部长通道”的采访，形容为记者与部长之间“一次非常亲密的问答”。作为一名已采访全国两会10余年的海外记者，娜塔莎目睹了“部长通道”的变迁。她认为，今年“部长通道”最大的特点就是搭起了一个媒体记者与部长们平等对话的平台。娜塔莎感叹，这种和谐活跃的氛围、平等对话的形式折射出中国政府执政亲民化的趋势，也展现出一种随和的大国形象。

                                                　　“开门办会”传递中国声音

                                                    　　“两会是了解中国发展的一扇窗户，政治、经济、民生，方方面面的话题都会在这里得到讨论和解答。”来自中非新闻交流中心的南苏丹记者阿汤加是第一次报道中国两会，他告诉记者，他最关心的是中国两会中会释放哪些与非洲政策、对非洲援助等有关的信息。他还非常关心中国的政治制度是如何运行的，“中国给非洲国家的发展提供了非常有价值的借鉴。”

                                                        　　俄罗斯塔斯社记者阿列克谢·谢里谢夫是一名资深驻华记者，在今年的两会外长记者会上，他就中俄美三国关系向外长提问。阿列克谢用流利的中文向记者感慨，“我感觉到中国正在变得更加自信和开放。我非常看好中国的稳定发展。”阿列克谢说。

                                                            　　日本朝日电视台的山本志门是第三次报道两会，他笑着告诉记者，报道中国两会是个“苦差事”。在短短十几天里，召开密集的记者会、出台各种新的政策，这样参与人数之多、影响之广的政治盛会，对于外国记者的报道来说是很大的考验。在今年的两会外长记者会上，山本提了有关“中日关系”的问题。他告诉本报记者，日本国内有非常多关于中国的报道，日本民众也很关心有关中国的各种话题。他希望日中两国能够加强沟通和了解。（本报记者 刘峰 杜一菲）
    '''
    import time
    start=time.time()
    for i in range(10000):
        model.add_word('么么么么吗','n',100)
    print 10000/(time.time()-start)

    for i in range(50):
        s=model.cut(text)
        print len(s)
    print len(text)*50/(time.time()-start)/1000
    
    start=time.time()
    for i in range(50):
        s=model.pos_cut(text)
    print len(s)
    print len(text)*50/(time.time()-start)/1000
    
    start=time.time()
    text='中国政治中国政治'
    for i in range(50):
        s=model.extract(text,30)
        print len(s)
    for i,j in s:
        print i,j
    print (time.time()-start)/50
    print len(text)*50/(time.time()-start)/1000
    
    start=time.time()
    for i in range(50):
        model.load_user_dicts('user_dicts/地方.txt')
    print (time.time()-start)/50
