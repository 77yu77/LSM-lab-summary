#include "kvstore.h"
/* the struct that write to the file */
struct Tablemessage {
  uint64_t timestamp;
  uint64_t keypairnum;
  uint64_t least;
  uint64_t largest;
  bitset<81920> BloomFilter;
  uint32_t size;
};

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir) {
  std::string nowdir;
  timestamp = 1;
  nowdir = dir;
  nowdir.append("/level ");
  maindir = nowdir;
  memtable = new Memtable();
  int i = 0;
  if (!utils::dirExists(nowdir.append("0"))) {
    utils::mkdir(nowdir.data());
    Memorydir memorydir;
    memorydir.dir = nowdir;
    SSTablebuffer.push_back(memorydir);
  } else {
    nowdir.pop_back();
    while (utils::dirExists(nowdir.append(std::to_string(i)))) {
      std::vector<std::string> files;
      Memorydir memorydir;
      memorydir.dir = nowdir;
      int length = utils::scanDir(nowdir, files);
      for (int j = 0; j < length; j++) {
        SSTableofmemory SStable;
        SStable.name = nowdir;
        SStable.name.append("/");
        SStable.name.append(files[j]);
        std::ifstream inFile(SStable.name, std::ios::in | std::ios::binary);
        Tablemessage *tablemessage = new Tablemessage;
        inFile.read((char *)tablemessage, Tablefixsize);
        index *Index = new index[tablemessage->keypairnum];
        inFile.read((char *)Index, tablemessage->keypairnum * 16);
        for (uint64_t k = 0; k < tablemessage->keypairnum; k++) {
          SStable.table.input(Index[k].key, Index[k].offset);
        }
        SStable.table.timestamp = tablemessage->timestamp;
        SStable.table.keypairnum = tablemessage->keypairnum;
        SStable.table.largest = tablemessage->largest;
        SStable.table.least = tablemessage->least;
        SStable.table.BloomFilter = tablemessage->BloomFilter;
        memorydir.SSTableline.push_back(SStable);
        inFile.close();
      }
      SSTablebuffer.push_back(memorydir);
      i++;
      nowdir.pop_back();
    }
    nowdir.pop_back();
    nowdir.append("0");
    std::vector<std::string> files;
    if (utils::scanDir(nowdir, files) != 0) {
      for (unsigned i = 0; i < SSTablebuffer[0].SSTableline.size(); i++) {
        if (SSTablebuffer[0].SSTableline[i].table.timestamp > timestamp) {
          timestamp = SSTablebuffer[0].SSTableline[i].table.timestamp;
        }
      }
      //        std::string
      //        data=seekfromdisk(0,t,0,SSTablebuffer[0].SSTableline[t].table.keygroup.back().offset);
      //        for(unsigned
      //        i=0;i<SSTablebuffer[0].SSTableline[t].table.keypairnum;i++){
      //            if(i==0){
      //                uint32_t
      //                length=SSTablebuffer[0].SSTableline[t].table.keygroup[0].offset;
      //                memtable->put(SSTablebuffer[0].SSTableline[t].table.keygroup[i].key,data.substr(0,length));
      //            }
      //              else{
      //                uint32_t
      //                length=SSTablebuffer[0].SSTableline[t].table.keygroup[i].offset-SSTablebuffer[0].SSTableline[t].table.keygroup[i-1].offset;
      //                memtable->put(SSTablebuffer[0].SSTableline[t].table.keygroup[i].key,data.substr(SSTablebuffer[0].SSTableline[t].table.keygroup[i-1].offset,length));
      //              }

      //        }
      //      utils::rmfile(SSTablebuffer[0].SSTableline[t].name.data());
      //      SSTablebuffer[0].SSTableline.erase(SSTablebuffer[0].SSTableline.begin()+t);
    } else {
      for (unsigned i = 0; i < SSTablebuffer[1].SSTableline.size(); i++) {
        if (timestamp < SSTablebuffer[1].SSTableline[i].table.timestamp)
          timestamp = SSTablebuffer[1].SSTableline[i].table.timestamp;
      }
    }
    timestamp++;
  }
}

KVStore::~KVStore() {
  if (memtable->returnnum() == 0)
    return;
  SSTableofmemory newone;
  newone.table.timestamp = timestamp;
  newone.table.keypairnum = memtable->returnnum();
  newone.table.least = memtable->returnleast();
  newone.table.largest = memtable->returnlargest();
  std::string ss = maindir;
  ss.append("0/");
  ss.append(std::to_string((int)timestamp)); //以时间戳命名文件
  newone.name = ss;
  std::ofstream outFile(ss, std::ios::out | std::ios::binary);
  timestamp++;
  Node *tmp = memtable->prihead;
  uint32_t position = 0;
  char *value = new char[2 * 1024 * 1024];
  value[0] = ' ';
  while (tmp->right) { //将key和offset导入内存
    tmp = tmp->right;
    position += tmp->value.length();
    newone.table.input(tmp->key, position);
    if (value[0] == ' ')
      strcpy(value, tmp->value.data());
    else
      strcat(value, tmp->value.data());
  }

  //将表信息写入磁盘
  Tablemessage tablemessage;
  tablemessage.timestamp = newone.table.timestamp;
  tablemessage.keypairnum = newone.table.keypairnum;
  tablemessage.least = newone.table.least;
  tablemessage.largest = newone.table.largest;
  tablemessage.BloomFilter = newone.table.BloomFilter;
  outFile.write((char *)&tablemessage, Tablefixsize);
  outFile.write((char *)&newone.table.keygroup[0],
                tablemessage.keypairnum * 16);
  outFile.write(value, newone.table.keygroup.back().offset);
  delete[] value;
  outFile.close();
  //将新的MemTable放进保存所有MemTable的内存里。
  SSTablebuffer[0].SSTableline.push_back(newone);
  if (SSTablebuffer[0].SSTableline.size() > 2) {
    compaction();
  }
}

/**
 * Compact the SSTable saved in the disk and memory.
 * No return.
 */
struct Position {
  uint32_t levelnum;
  uint32_t SSTablenum;
};
unsigned mi(unsigned a, unsigned b) {
  unsigned t = a;
  for (unsigned i = 1; i < b; i++)
    a *= t;
  return a;
}
void KVStore::compaction() {
  unsigned levelnum = 0;
  unsigned int size = SSTablebuffer[0].SSTableline.size(); //当前层的table数
  int downnum = 0; //需要放到下一层level的table数
  while ((downnum = size - (mi(2, levelnum + 1))) > 0) {
    std::vector<Position> tocompaction; //存放将要放到下一层的table的所在位置
    std::vector<std::string> tocompactiondatas; //存放将要合并的所有value
    std::vector<std::string> nowleveldelete; //当前层将要被删除的文件
    uint64_t largestkey = 0;
    uint64_t leastkey = LONG_LONG_MAX;
    uint64_t lasttimestamp = LONG_LONG_MAX;
    //取第0层的所有数据
    if (levelnum == 0) {
      tocompaction.push_back({0, 0});
      tocompaction.push_back({0, 1});
      tocompaction.push_back({0, 2});
      nowleveldelete.push_back(SSTablebuffer[0].SSTableline[0].name);
      nowleveldelete.push_back(SSTablebuffer[0].SSTableline[1].name);
      nowleveldelete.push_back(SSTablebuffer[0].SSTableline[2].name);
      //取三个表合起来的最大值与最小值
      largestkey = (SSTablebuffer[0].SSTableline[0].table.largest >
                            SSTablebuffer[0].SSTableline[1].table.largest
                        ? SSTablebuffer[0].SSTableline[0].table.largest
                        : SSTablebuffer[0].SSTableline[1].table.largest);
      largestkey = (SSTablebuffer[0].SSTableline[1].table.largest >
                            SSTablebuffer[0].SSTableline[2].table.largest
                        ? SSTablebuffer[0].SSTableline[1].table.largest
                        : SSTablebuffer[0].SSTableline[2].table.largest);
      leastkey = (SSTablebuffer[0].SSTableline[0].table.least <
                          SSTablebuffer[0].SSTableline[1].table.least
                      ? SSTablebuffer[0].SSTableline[0].table.least
                      : SSTablebuffer[0].SSTableline[1].table.least);
      leastkey = (leastkey < SSTablebuffer[0].SSTableline[2].table.least
                      ? leastkey
                      : SSTablebuffer[0].SSTableline[2].table.least);
      lasttimestamp = SSTablebuffer[0].SSTableline[2].table.timestamp;
    } else {
      lasttimestamp =
          SSTablebuffer[levelnum].SSTableline[downnum].table.timestamp;
      downnum--;
      unsigned t = 0;
      while (downnum >= 0) {
        tocompaction.push_back({levelnum, t});
        nowleveldelete.push_back(SSTablebuffer[levelnum].SSTableline[t].name);
        largestkey =
            (SSTablebuffer[levelnum].SSTableline[t].table.largest > largestkey
                 ? SSTablebuffer[levelnum].SSTableline[t].table.largest
                 : largestkey);
        leastkey =
            (SSTablebuffer[levelnum].SSTableline[t].table.least < leastkey
                 ? SSTablebuffer[levelnum].SSTableline[t].table.least
                 : leastkey);
        t += 1;
        downnum--;
      }
    }
    std::vector<std::string> deletetable;
    //取下一层的符合数据
    levelnum++;
    std::string nextlevelname = maindir;
    nextlevelname.append(std::to_string(levelnum));
    if (!utils::dirExists(nextlevelname)) {
      mkdir(nextlevelname.data());
      Memorydir newline;
      std::string k = "level";
      k.append(std::to_string(levelnum));
      newline.dir = k;
      SSTablebuffer.push_back(newline);
    } else {
      unsigned int nextlevellength = SSTablebuffer[levelnum].SSTableline.size();
      for (unsigned int i = 0; i < nextlevellength; ++i) {
        if (SSTablebuffer[levelnum].SSTableline[i].table.largest < leastkey ||
            SSTablebuffer[levelnum].SSTableline[i].table.least > largestkey)
          continue;
        //取最大的timestamp
        lasttimestamp =
            (SSTablebuffer[levelnum].SSTableline[i].table.timestamp >
                     lasttimestamp
                 ? SSTablebuffer[levelnum].SSTableline[i].table.timestamp
                 : lasttimestamp);
        deletetable.push_back(SSTablebuffer[levelnum].SSTableline[i].name);
        tocompaction.push_back({levelnum, i});
      }
    }
    uint32_t tocompactionlength = tocompaction.size();
    for (unsigned i = 0; i < tocompactionlength; i++) {
      tocompactiondatas.push_back(
          seekfromdisk(tocompaction[i].levelnum, tocompaction[i].SSTablenum, 0,
                       SSTablebuffer[tocompaction[i].levelnum]
                           .SSTableline[tocompaction[i].SSTablenum]
                           .table.keygroup.back()
                           .offset));
      //       uint32_t t=tocompactiondatas[i].length();
      //       if(t==SSTablebuffer[tocompaction[i].levelnum].SSTableline[tocompaction[i].SSTablenum].table.keygroup.back().offset)
      //        std::cout<<1<<std::endl;
      //       else
      //         std::cout<<0<<std::endl;
    }

    //所有表合并的位置
    std::vector<uint64_t> position;
    std::vector<uint64_t> maxposition;
    //最大位置
    for (unsigned int i = 0; i < tocompactionlength; i++) {
      maxposition.push_back((SSTablebuffer[tocompaction[i].levelnum]
                                 .SSTableline[tocompaction[i].SSTablenum]
                                 .table.keypairnum));
    }
    //当前位置初始化
    for (unsigned int i = 0; i < tocompactionlength; ++i)
      position.push_back(0);
    //循环输出SSTable
    while (true) {
      bool isfinish = false;
      SSTableofmemory newone;
      newone.table.timestamp = lasttimestamp;
      uint64_t pairnum = 0;
      char *value = new char[2 * 1024 * 1024];
      value[0] = ' ';
      uint32_t alllength = 0;
      uint64_t wholesize = Tablefixsize;
      std::string ss = maindir;
      ss.append(std::to_string(levelnum));
      ss.push_back('/');
      ss.append(std::to_string(lasttimestamp));
      ss.push_back('.');
      leastkey = LONG_LONG_MAX;

      //取出最小key
      for (unsigned i = 0; i < tocompactionlength; i++) {
        if (position[i] == maxposition[i])
          continue;
        if (leastkey > SSTablebuffer[tocompaction[i].levelnum]
                           .SSTableline[tocompaction[i].SSTablenum]
                           .table.keygroup[position[i]]
                           .key)
          leastkey = SSTablebuffer[tocompaction[i].levelnum]
                         .SSTableline[tocompaction[i].SSTablenum]
                         .table.keygroup[position[i]]
                         .key;
      }
      newone.table.least = leastkey;
      ss.append(std::to_string(leastkey));
      newone.name = ss;
      //合并table
      while (true) {
        leastkey = LONG_LONG_MAX;
        //取出当前position最小的key
        for (unsigned int i = 0; i < tocompactionlength; i++) {
          if (position[i] == maxposition[i])
            continue;
          if (SSTablebuffer[tocompaction[i].levelnum]
                  .SSTableline[tocompaction[i].SSTablenum]
                  .table.keygroup[position[i]]
                  .key < leastkey) {
            leastkey = SSTablebuffer[tocompaction[i].levelnum]
                           .SSTableline[tocompaction[i].SSTablenum]
                           .table.keygroup[position[i]]
                           .key;
          }
        }
        uint64_t newposition = 0;
        uint64_t largesttimestamp = 0;
        //取出最小key对应的最大timestamp及其对应位置
        for (unsigned int i = 0; i < tocompactionlength; i++) {
          if (position[i] == maxposition[i])
            continue;
          if (leastkey == SSTablebuffer[tocompaction[i].levelnum]
                              .SSTableline[tocompaction[i].SSTablenum]
                              .table.keygroup[position[i]]
                              .key) {
            if (largesttimestamp < SSTablebuffer[tocompaction[i].levelnum]
                                       .SSTableline[tocompaction[i].SSTablenum]
                                       .table.timestamp) {
              newposition = i;
              largesttimestamp = SSTablebuffer[tocompaction[i].levelnum]
                                     .SSTableline[tocompaction[i].SSTablenum]
                                     .table.timestamp;
            }
          }
        }

        //取对应位置的key和value并保存
        std::string result;
        uint32_t length;
        if (position[newposition] > 0) {
          length = SSTablebuffer[tocompaction[newposition].levelnum]
                       .SSTableline[tocompaction[newposition].SSTablenum]
                       .table.keygroup[position[newposition]]
                       .offset -
                   SSTablebuffer[tocompaction[newposition].levelnum]
                       .SSTableline[tocompaction[newposition].SSTablenum]
                       .table.keygroup[position[newposition] - 1]
                       .offset;
          result = tocompactiondatas[newposition].substr(
              SSTablebuffer[tocompaction[newposition].levelnum]
                  .SSTableline[tocompaction[newposition].SSTablenum]
                  .table.keygroup[position[newposition] - 1]
                  .offset,
              length);
          wholesize += 16 + length;
        } else {
          length = SSTablebuffer[tocompaction[newposition].levelnum]
                       .SSTableline[tocompaction[newposition].SSTablenum]
                       .table.keygroup[position[newposition]]
                       .offset;
          result = tocompactiondatas[newposition].substr(0, length);
          wholesize += length + 16;
        }
        //大于2MB，输出table
        if (wholesize > MAXSIZE) {
          //将position倒退回去
          //满足2MB，生成table
          newone.table.keypairnum = pairnum;
          newone.table.largest = newone.table.keygroup.back().key;
          SSTablebuffer[levelnum].SSTableline.push_back(newone);
          std::ofstream outFile(newone.name, std::ios::out | std::ios::binary);
          Tablemessage tablemessage;
          tablemessage.timestamp = newone.table.timestamp;
          tablemessage.keypairnum = newone.table.keypairnum;
          tablemessage.least = newone.table.least;
          tablemessage.largest = newone.table.largest;
          tablemessage.BloomFilter = newone.table.BloomFilter;
          outFile.write((char *)&tablemessage, Tablefixsize);
          outFile.write((char *)&newone.table.keygroup[0],
                        tablemessage.keypairnum * 16);
          outFile.write(value, newone.table.keygroup.back().offset);
          delete[] value;
          outFile.close();
          break;
        }
        //增加当前位置key值为leastkey的Table的位置
        for (unsigned int i = 0; i < tocompactionlength; i++) {
          if (position[i] == maxposition[i])
            continue;
          if (leastkey == SSTablebuffer[tocompaction[i].levelnum]
                              .SSTableline[tocompaction[i].SSTablenum]
                              .table.keygroup[position[i]]
                              .key) {
            position[i]++;
          }
        }
        pairnum++;
        alllength += length;
        newone.table.input(leastkey, alllength);

        if (value[0] == ' ')
          strcpy(value, result.data());
        else
          strcat(value, result.data());
        //完成合并
        bool flag = true;
        for (unsigned i = 0; i < tocompactionlength; i++) {
          if (position[i] != maxposition[i]) {
            flag = false;
            break;
          }
        }

        if (flag) {
          newone.table.keypairnum = pairnum;
          newone.table.largest = newone.table.keygroup.back().key;
          std::ofstream outFile(newone.name, std::ios::out | std::ios::binary);
          SSTablebuffer[levelnum].SSTableline.push_back(newone);
          Tablemessage tablemessage;
          tablemessage.timestamp = newone.table.timestamp;
          tablemessage.keypairnum = newone.table.keypairnum;
          tablemessage.least = newone.table.least;
          tablemessage.largest = newone.table.largest;
          tablemessage.BloomFilter = newone.table.BloomFilter;
          outFile.write((char *)&tablemessage, Tablefixsize);

          outFile.write((char *)&newone.table.keygroup[0],
                        tablemessage.keypairnum * 16);
          outFile.write(value, newone.table.keygroup.back().offset);
          outFile.close();
          delete[] value;
          isfinish = true;
          break;
        }
      }
      if (isfinish) {
        unsigned length = nowleveldelete.size();
        for (unsigned i = 0; i < length; i++) {
          unsigned j = 0;
          while (true) {
            if (nowleveldelete[i] ==
                SSTablebuffer[levelnum - 1].SSTableline[j].name) {
              std::string t = nowleveldelete[i];
              utils::rmfile(
                  SSTablebuffer[levelnum - 1].SSTableline[j].name.data());
              SSTablebuffer[levelnum - 1].SSTableline.erase(
                  SSTablebuffer[levelnum - 1].SSTableline.begin() + j);
              break;
            }
            j++;
          }
        }
        for (unsigned i = 0; i < deletetable.size(); i++) {
          unsigned j = 0;
          while (true) {
            if (deletetable[i] == SSTablebuffer[levelnum].SSTableline[j].name) {
              utils::rmfile(SSTablebuffer[levelnum].SSTableline[j].name.data());
              SSTablebuffer[levelnum].SSTableline.erase(
                  SSTablebuffer[levelnum].SSTableline.begin() + j);
              break;
            }
            j++;
          }
        }
        break;
      }
    }

    size = SSTablebuffer[levelnum].SSTableline.size();
  }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
char *KVStore::seekfromdisk(unsigned int levelnum, unsigned int SSTablenum,
                            uint32_t start, uint32_t end) {
  std::string s;
  s = SSTablebuffer[levelnum].SSTableline[SSTablenum].name;
  std::ifstream inFile(s, std::ios::in | std::ios::binary);
  char *now = new char[end - start + 1];
  uint32_t len =
      Tablefixsize +
      SSTablebuffer[levelnum].SSTableline[SSTablenum].table.keypairnum * 16 +
      start;
  inFile.seekg(len);
  inFile.read(now, (end - start));
  inFile.close();
  now[end - start] = '\0';
  return now;
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {

  if (!memtable->put(key, s)) { //插入后超过2MB，故存入磁盘
    SSTableofmemory newone;
    newone.table.timestamp = timestamp;
    newone.table.keypairnum = memtable->returnnum();
    newone.table.least = memtable->returnleast();
    newone.table.largest = memtable->returnlargest();
    std::string ss = maindir;
    ss.append("0/");
    ss.append(std::to_string((int)timestamp)); //以时间戳命名文件
    newone.name = ss;
    std::ofstream outFile(ss, std::ios::out | std::ios::binary);
    timestamp++;
    Node *tmp = memtable->prihead;
    uint32_t position = 0;
    char *value = new char[2 * 1024 * 1024];
    value[0] = ' ';
    while (tmp->right) { //将key和offset导入内存
      tmp = tmp->right;
      position += tmp->value.length();
      newone.table.input(tmp->key, position);
      if (value[0] == ' ')
        strcpy(value, tmp->value.data());
      else
        strcat(value, tmp->value.data());
    }

    //将表信息写入磁盘
    Tablemessage tablemessage;
    tablemessage.timestamp = newone.table.timestamp;
    tablemessage.keypairnum = newone.table.keypairnum;
    tablemessage.least = newone.table.least;
    tablemessage.largest = newone.table.largest;
    tablemessage.BloomFilter = newone.table.BloomFilter;
    outFile.write((char *)&tablemessage, Tablefixsize);
    outFile.write((char *)&newone.table.keygroup[0],
                  tablemessage.keypairnum * 16);
    outFile.write(value, newone.table.keygroup.back().offset);
    delete[] value;
    outFile.close();
    //将新的MemTable放进保存所有MemTable的内存里。
    SSTablebuffer[0].SSTableline.push_back(newone);
    if (SSTablebuffer[0].SSTableline.size() > 2) {
      compaction();
    }
    memtable->clear();
    delete memtable;
    memtable = new Memtable;
    memtable->put(key, s);
  }
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
  std::string t;
  if ((t = memtable->get(key)) != "") {
    if (t == "~DELETED~")
      return "";
    return t;
  }
  unsigned int i = 0;
  unsigned int levelnum = SSTablebuffer.size();
  while (i < levelnum) {
    unsigned int j = 0;
    unsigned int SSTablenum = SSTablebuffer[i].SSTableline.size();
    while (j < SSTablenum) {

      //   只保存文件名字，不使用布隆过滤器在磁盘查找
      //       std::ifstream
      //       inFile(SSTablebuffer[i].SSTableline[j].name,std::ios::in|std::ios::binary);
      //       inFile.seekg(Tablefixsize);
      //       index *Index=new
      //       index[SSTablebuffer[i].SSTableline[j].table.keypairnum];
      //       inFile.read((char
      //       *)Index,SSTablebuffer[i].SSTableline[j].table.keypairnum*16);{
      //           unsigned int left=0;
      //           unsigned int
      //           right=SSTablebuffer[i].SSTableline[j].table.keypairnum; while
      //           (true) {            //在内存二分查找是否有key
      //               if(left>=right){
      //                   if(Index[left].key==key){
      //                       uint32_t start;
      //                       uint32_t end=Index[left].offset;
      //                       if(left==0)
      //                           start=0;
      //                       else
      //                           start=Index[left-1].offset;
      //                       std::string t=seekfromdisk(i,j,start,end);
      //                       if(t=="~DELETED~")
      //                           return "";
      //                         return t;
      //                   }
      //                   break;
      //                 }

      //               unsigned int mid=(left+right)/2;
      //               if(Index[mid].key==key){
      //                   uint32_t start;
      //                   uint32_t end=Index[mid].offset;
      //                   if(mid==0)
      //                       start=0;
      //                   else
      //                       start=Index[mid-1].offset;
      //                   std::string t=seekfromdisk(i,j,start,end);
      //                   if(t=="~DELETED~")
      //                       return "";
      //                   return t;
      //               }
      //              if(key>Index[mid].key)
      //                 left=mid+1;
      //              else {
      //                  if(mid==0)
      //                     break;
      //                  right=mid-1;
      //              }
      //           }
      //       }
      //采用布隆过滤器
      if (SSTablebuffer[i].SSTableline[j].table.Bloomsearch(key)) {
        //是否添加判断最大键值和最小键值
        unsigned int left = 0;
        unsigned int right = SSTablebuffer[i].SSTableline[j].table.keypairnum;
        while (true) { //在内存二分查找是否有key
          if (left >= right) {
            if (SSTablebuffer[i].SSTableline[j].table.keygroup[left].key ==
                key) {
              uint32_t start;
              uint32_t end =
                  SSTablebuffer[i].SSTableline[j].table.keygroup[left].offset;
              if (left == 0)
                start = 0;
              else
                start = SSTablebuffer[i]
                            .SSTableline[j]
                            .table.keygroup[left - 1]
                            .offset;
              std::string t = seekfromdisk(i, j, start, end);
              if (t == "~DELETED~")
                return "";
              return t;
            }
            break;
          }

          unsigned int mid = (left + right) / 2;
          if (SSTablebuffer[i].SSTableline[j].table.keygroup[mid].key == key) {
            uint32_t start;
            uint32_t end =
                SSTablebuffer[i].SSTableline[j].table.keygroup[mid].offset;
            if (mid == 0)
              start = 0;
            else
              start = SSTablebuffer[i]
                          .SSTableline[j]
                          .table.keygroup[mid - 1]
                          .offset;
            std::string t = seekfromdisk(i, j, start, end);
            if (t == "~DELETED~")
              return "";
            return t;
          }
          if (key > SSTablebuffer[i].SSTableline[j].table.keygroup[mid].key)
            left = mid + 1;
          else {
            if (mid == 0)
              break;
            right = mid - 1;
          }
        }
      }
      j++;
    }

    i++;
  }
  return "";
}
uint32_t KVStore::judgeinmemory(const uint64_t key) {
  unsigned int i = 0;
  unsigned int levelnum = SSTablebuffer.size();
  while (i < levelnum) {
    unsigned int j = 0;
    unsigned int SSTablenum = SSTablebuffer[i].SSTableline.size();
    while (j < SSTablenum) {

      if (SSTablebuffer[i].SSTableline[j].table.Bloomsearch(key)) {
        //是否添加判断最大键值和最小键值
        unsigned int left = 0;
        unsigned int right =
            SSTablebuffer[i].SSTableline[j].table.keygroup.size();
        while (true) { //在内存二分查找是否有key
          if (left >= right) {
            if (SSTablebuffer[i].SSTableline[j].table.keygroup[left].key ==
                key) {
              if (left == 0)
                return SSTablebuffer[i]
                    .SSTableline[j]
                    .table.keygroup[left]
                    .offset;
              return (
                  SSTablebuffer[i].SSTableline[j].table.keygroup[left].offset -
                  SSTablebuffer[i]
                      .SSTableline[j]
                      .table.keygroup[left - 1]
                      .offset);
            }
            break;
          }
          unsigned int mid = (left + right) / 2;
          if (SSTablebuffer[i].SSTableline[j].table.keygroup[mid].key == key) {
            if (mid == 0)
              return SSTablebuffer[i].SSTableline[j].table.keygroup[mid].offset;
            return (
                SSTablebuffer[i].SSTableline[j].table.keygroup[mid].offset -
                SSTablebuffer[i].SSTableline[j].table.keygroup[mid - 1].offset);
          }
          if (key > SSTablebuffer[i].SSTableline[j].table.keygroup[mid].key)
            left = mid + 1;
          else {
            if (mid == 0)
              break;
            right = mid - 1;
          }
        }
      }
      j++;
    }
    i++;
  }
  return 0;
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
  std::string t;
  uint32_t k = 0;
  if ((t = memtable->get(key)) != "") {
    if (t == "~DELETED~")
      return false;
    memtable->del(key);
    return true;
  } else if ((k = judgeinmemory(key)) == 9) {
    if (get(key) == "")
      return false;
    put(key, "~DELETED~");
    return true;
  }
  if (k == 0)
    return false;
  put(key, "~DELETED~");
  return true;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
  std::string nowdir = maindir;
  int i = 0;
  while (utils::dirExists(nowdir.append(std::to_string(i)))) {
    std::vector<std::string> files;
    int length = utils::scanDir(nowdir, files);
    for (int j = 0; j < length; j++) {
      std::string filename = nowdir;
      filename.append("/");
      filename.append(files[j]);
      utils::rmfile(filename.data());
    }
    utils::rmdir(nowdir.data());
    nowdir.pop_back();
    i++;
  }
  memtable->clear();
  delete memtable;
  memtable = new Memtable;
  std::vector<Memorydir>().swap(SSTablebuffer);
  nowdir = maindir;
  nowdir.append("0");
  timestamp = 1;
  utils::mkdir(nowdir.data());
  Memorydir memorydir;
  memorydir.dir = nowdir;
  SSTablebuffer.push_back(memorydir);
}
