#pragma once
#include "MemTable.h"
#include "MurmurHash3.h"
#include "kvstore_api.h"
#include <bitset>

using std::bitset;
#include "utils.h"
#include <fstream>
#include <iostream>
#define Tablefixsize 10280
struct index { //索引key与offset
  uint64_t key;
  uint32_t offset;
};

struct SSTable {
  uint64_t timestamp;
  uint64_t keypairnum;
  uint64_t least;
  uint64_t largest;
  bitset<81920> BloomFilter;
  std::vector<index> keygroup;
  SSTable(uint64_t timestamp, uint64_t keypairnum, uint64_t least,
          uint64_t largest)
      : timestamp(timestamp), keypairnum(keypairnum), least(least),
        largest(largest) {}
  SSTable(){};
  void input(uint64_t key, uint32_t offset) {
    //插入时改变布隆过滤器
    unsigned int hash[4] = {0};
    uint64_t t = key;
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (int i = 0; i < 4; ++i)
      hash[i] = hash[i] % 81920;
    BloomFilter.set(hash[0]);
    BloomFilter.set(hash[1]);
    BloomFilter.set(hash[2]);
    BloomFilter.set(hash[3]);
    keygroup.push_back({t, offset});
  }

  bool Bloomsearch(const uint64_t key) { //通过布隆过滤器查询key是否在
    unsigned int hash[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (int i = 0; i < 4; ++i)
      hash[i] = hash[i] % 81920;
    for (int i = 0; i < 4; ++i)
      if (!BloomFilter.test(hash[i]))
        return false;
    return true;
  }
};
struct SSTableofmemory {
  std::string name;
  SSTable table;
};

struct Memorydir { //内存中的directory
  std::string dir = "";
  std::vector<SSTableofmemory> SSTableline;
};

class KVStore : public KVStoreAPI {
  // You can add your implementation here
private:
  Memtable *memtable;
  uint64_t timestamp;
  std::vector<Memorydir> SSTablebuffer;
  std::string maindir;

public:
  KVStore(const std::string &dir);
  ~KVStore();
  void put(uint64_t key, const std::string &s) override;
  std::string get(uint64_t key) override;
  bool del(uint64_t key) override;
  uint32_t judgeinmemory(const uint64_t key);

  void compaction(); //合并函数
  char *seekfromdisk(unsigned int levelnum, unsigned int SSTablenum,
                     uint32_t start, uint32_t end); //从磁盘读取字符串
  void seekfromdisk2(unsigned int levelnum, unsigned int SSTablenum,
                     uint32_t start, uint32_t end);

  void reset() override;
};
