#include <iostream>
#include <fstream>
#include "kvstore.h"
using namespace std;

//test the input/output is right or not 
int main()
{
 KVStore lsm("ssss");
 uint64_t num=1;
 string s="ss";
 while (num<10) {
   lsm.put(num,s);
   s.push_back('S');
   num++;
 }
//  cout<<lsm.get(2)<<endl<<lsm.get(7)<<endl;
//  cout<<lsm.del(2)<<endl;
//  cout<<lsm.get(2)<<endl;
  utils::mkdir("./store/level 0");
  cout<<utils::dirExists("./store/level 0");
  std::string s;
  s="./store/level 1";
  mkdir(s.data());
  s.append(".");
  unsigned int i=1;
  s.append(std::to_string(i));
  std::ofstream outFile(s,std::ios::out | std::ios::binary);
  char t[]="ssssttss";
  outFile.write(t,sizeof (t));
  outFile.close();
  std::ifstream inFile(s,std::ios::in|std::ios::binary);
  char a[4];
  inFile.read((char *)&a, 4);
  for(int i=0;i<4;++i)
  cout<<a[i];
  inFile.read((char *)&a,2);
  std::cout<<a<<endl;
  inFile.close();
}
