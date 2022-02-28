#include "MemTable.h"



bool Memtable::put(const uint64_t key,const std::string value){
    std::vector<Node*> pathList;    //从上至下记录搜索路径
            Node *p = head;
            while(p){
                while(p->right &&(p->right->key < key)){
                    p = p->right;
                }
                if(p->right&&(p->right->key==key)){
                    p=p->right;
                    size-=p->value.length();
                    size+=value.length();
                    if(size>=MAXSIZE){
                        size+=p->value.length()-value.length();
                        return false;
                    }
                    while(p){
                        p->value=value;
                        p=p->down;
                    }
                    return true;
                }
                pathList.push_back(p);
                p = p->down;
            }

            size=size+value.length()+16;
            if(size>=MAXSIZE){
               size=size-value.length()-16;
               return false;
            }
            if(key<least)
                least=key;
            else if(key>largest)
                largest=key;
            num++;
            bool insertUp = true;
            Node* downNode= nullptr;
            while(insertUp && pathList.size() > 0){   //从下至上搜索路径回溯，50%概率
                Node *insert = pathList.back();               
                insert->right = new Node(insert->right, downNode, key, value); //add新结点
                downNode = insert->right;    //把新结点赋值为downNode
                insertUp = (rand()&1);   //50%概率
                pathList.pop_back();
            }
            if(insertUp){  //插入新的头结点，加层
                Node* oldHead = head;
                head = new Node();
                head->right = new Node(nullptr, downNode, key, value);
                head->down = oldHead;
            }
            return true;
}

std::string Memtable::get(const uint64_t key){
           Node *p = head;
           std::string answer="";
           while(p){
             while(p->right&&(p->right->key < key))
                p=p->right;       //找key对应的塔
            if(!p->right||(p->right->key > key)){
               p=p->down;
               continue;
             }
              answer=p->right->value;
              return answer;
           }
          return answer;
}

bool Memtable::del(const uint64_t key){
          Node *p = head;
          while(true){
             if(!p)
                 return false;
            while(p->right&&(p->right->key < key))
               p=p->right;       //找key对应的塔
           if(!(p->right)||(p->right->key > key)){
              p=p->down;
              continue;
            }
           if(p->right&&p->right->key==key){
              p=p->right;
              break;
           }
          }
        if(p->value=="~DELETED~")  //该点已删除
            return false;
          size=size-p->value.length();
          size+=9;
//          if(size>=MAXSIZE){
//             size=size-9+p->value.length();
//             return false;
//          }
          while (p) {
              p->value="~DELETED~";
              p=p->down;
          }
          return true;
    }

uint64_t Memtable::returnsize(){
    return size;
}

uint64_t Memtable::returnnum(){
    return num;
}

uint64_t Memtable::returnleast(){
    return least;
}

uint64_t Memtable::returnlargest(){
    return largest;
}
void Memtable::clear(){
    Node *tmp=head;
    while (tmp) {
        Node *tmp1=tmp->down;
        while(tmp){
            Node *tmp2=tmp;
            tmp=tmp->right;
            delete tmp2;
        }

        tmp=tmp1;
    }
    head= new Node();
    size=10280;
    num=0;
    least=LONG_LONG_MAX;
    largest=0;
    prihead=head;
}
