#ifndef MEMTABLE_H
#define MEMTABLE_H
#include <cstdint>
#include <string>
#include <vector>
#define MAXSIZE 2097152
struct Node{
    Node *right=nullptr,*down=nullptr;
    uint64_t key;
    std::string value;
     Node(Node *right,Node *down,uint64_t key,std::string value): right(right), down(down), key(key), value(value){}
     Node(): right(nullptr), down(nullptr) {}
};


class Memtable{
private:

     uint64_t size;             //数据大小
     uint64_t num;              //键值对数
     uint64_t least;             //最小键值
     uint64_t largest;           //最大键值
     Node *head;
public:
     Node *prihead;
     Memtable(){
         head= new Node();
         prihead=head;
         size=10280;
         num=0;
         least=LONG_LONG_MAX;
         largest=0;
     }
     bool put(const uint64_t key,const std::string value);
     std::string get(const uint64_t key);
     bool del(const uint64_t key);
     uint64_t returnsize();
     uint64_t returnnum();
     uint64_t returnleast();
     uint64_t returnlargest();
     void clear();
     ~Memtable(){
     }
};

#endif // MEMTABLE_H
