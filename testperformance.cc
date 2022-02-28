#include <iostream>
#include <cstdint>
#include <string>
#include <time.h>
#include <fstream>
#include <windows.h>
#include "test.h"
using namespace std;
class PerformanceTest : public Test {
private:
    void regular_test(uint64_t max, uint64_t size)
    {
        uint64_t i;
//        clock_t start, finish;
        store.reset();
        double time=0;
        uint64_t num=0;
        uint64_t prenum=0;
        uint32_t second=1;
        LARGE_INTEGER Freq;
        LARGE_INTEGER BeginTime;
        LARGE_INTEGER EndTime;
        cout << "max: " << max << " size: " << size << std::endl;


        // Test 1 second thruput
        for (i = 0; i < max; ++i) {
            QueryPerformanceFrequency(&Freq);
            QueryPerformanceCounter(&BeginTime);//开始计时
            store.put(i, std::string(size, 's'));
            QueryPerformanceCounter(&EndTime);
            time+=(double)(EndTime.QuadPart-BeginTime.QuadPart)/(double)Freq.QuadPart;
            num+=1;
            if(time>=second){
               second++;
               cout<<(num-prenum)<<endl;
               prenum=num;
            }
        }

    //    store.reset();
    //    double time=0;
    //    LARGE_INTEGER Freq;
    //    LARGE_INTEGER BeginTime;
    //    LARGE_INTEGER EndTime;
    //    cout << "max: " << max << " size: " << size << std::endl;
    //    QueryPerformanceFrequency(&Freq);
    //    QueryPerformanceCounter(&BeginTime);//开始计时
    
//        // Test all put
//        for (i = 0; i < max; ++i) {
//            store.put(i, std::string(size, 's'));
//        }
//        QueryPerformanceCounter(&EndTime);//停止计时
//        time=(double)(EndTime.QuadPart-BeginTime.QuadPart)/(double)Freq.QuadPart;//计算程序执行时间单位为s
//        std::cout << "all PUT costs :"<< std::endl;
//        std::cout << time<<"s"<< std::endl;
//        cout<<"one PUT costs :"<<std::endl;
//        std::cout<<(time/max)<<"s"<<std::endl;
//        cout<<"PUT thruput:"<<std::endl;
//        std::cout<<(max/time)<<std::endl;

//        // Test after all gets
//        QueryPerformanceFrequency(&Freq);
//        QueryPerformanceCounter(&BeginTime);//开始计时
//        for (i = 0; i < max; ++i){
//            EXPECT(std::string(size, 's'), store.get(i));
//        }
//        QueryPerformanceCounter(&EndTime);//停止计时
//        time=(double)(EndTime.QuadPart-BeginTime.QuadPart)/(double)Freq.QuadPart;//计算程序执行时间单位为s
//        phase();
//        cout << "all Get costs :" <<endl;
//        cout<<time << "s" << std::endl;
//        cout<<"one Get costs :"<<std::endl;
//        std::cout << (time/max)<<"s"<< std::endl;
//        cout<<"Get thruput:"<<std::endl;
//        std::cout<<(max/time)<<std::endl;

        // Test after all Delections
//        QueryPerformanceFrequency(&Freq);
//        QueryPerformanceCounter(&BeginTime);//开始计时
//        // Test deletions
//        for (i = 0; i < max; i++) {
//            EXPECT(true, store.del(i));
//        }
//        QueryPerformanceCounter(&EndTime);//停止计时
//        time=(double)(EndTime.QuadPart-BeginTime.QuadPart)/(double)Freq.QuadPart;//计算程序执行时间单位为s
//        phase();
//       std::cout << "Delete all costs :" <<endl;
//       cout<<time << "s"<< std::endl;
//       cout<<"one Delete costs :"<<std::endl;
//       std::cout << (time/max)<<"s"<< std::endl;
//       cout<<"Delete thruput:"<<std::endl;
//       std::cout<<(max/time)<<std::endl;
       report();
    }
//    void random_test(uint64_t max, uint64_t size)
//    {
//        uint64_t i;
//        vector<int> len;
//      clock_t start, finish;
//        store.reset();
//        double time=0;
//        double counts=0;
//        LARGE_INTEGER nFreq;
//        LARGE_INTEGER nBeginTime;
//        LARGE_INTEGER nEndTime;
//        for (i = 0; i < max; ++i) {
//            len.push_back(rand()%(size/8)+size);
//        }
//        fout << "max: " << max << " size: " << size << std::endl;
//        QueryPerformanceFrequency(&nFreq);
//        QueryPerformanceCounter(&nBeginTime);//开始计时
//        // Test multiple key-value pairs
//        for (i = 0; i < max; ++i) {
//            store.put(i, std::string(len[i], 's'));
//        }
//        QueryPerformanceCounter(&nEndTime);//停止计时
//        time=(double)(nEndTime.QuadPart-nBeginTime.QuadPart)/(double)nFreq.QuadPart;//计算程序执行时间单位为s
//        phase();
//        fout << time << '\t';
//        std::cout << time << std::endl;

//        // Test after all insertions
//        QueryPerformanceFrequency(&nFreq);
//        QueryPerformanceCounter(&nBeginTime);//开始计时
//        for (i = 0; i < max; ++i){
//            EXPECT(std::string(len[i], 's'), store.get(i));
//        }
//        QueryPerformanceCounter(&nEndTime);//停止计时
//        time=(double)(nEndTime.QuadPart-nBeginTime.QuadPart)/(double)nFreq.QuadPart;//计算程序执行时间单位为s
//        phase();
//        fout << time << '\t';
//        std::cout << time << std::endl;

//        // Test after all insertions
//        QueryPerformanceFrequency(&nFreq);
//        QueryPerformanceCounter(&nBeginTime);//开始计时
//        // Test deletions
//        for (i = 0; i < max; i++) {
//            EXPECT(true, store.del(i));
//        }
//        QueryPerformanceCounter(&nEndTime);//停止计时
//        time=(double)(nEndTime.QuadPart-nBeginTime.QuadPart)/(double)nFreq.QuadPart;//计算程序执行时间单位为s
//        phase();
//        fout << time << std::endl << std::endl;
//        std::cout << time << std::endl;

//        report();
//    }

public:
    PerformanceTest(const std::string &dir, bool v=true) : Test(dir, v)
    {
    }

    void start_test(void *args = NULL) override
    {
        store.reset();
        std::cout << "KVStore Performance Test" << std::endl;
//		std::cout << "[Simple Test]" << std::endl;
//        regular_test(SIMPLE_TEST_MAX);

//        std::cout << "[Large Test]" << std::endl;
//        std::cout<<"Get,Put,Delete delay "<<endl;
//        regular_test(1024, 1024);
//        regular_test(2048, 1024);
//        regular_test(4096, 1024);
//        regular_test(8192, 1024);
//        regular_test(16384, 1024);
//        regular_test(32768, 1024);
//        regular_test(65536, 1024);
//        regular_test(131072, 1024);
//        regular_test(262144, 1024);
//        regular_test(524288, 1024);
//        regular_test(1024*2, 256);
//        regular_test(2048*2, 256);
//        regular_test(4096*2, 256);
//        regular_test(8192*2, 256);
//        regular_test(16384*2, 256);
//        regular_test(32768*2, 256);
//        regular_test(65536*2, 256);
//        regular_test(131072*2, 256);
//        regular_test(262144*2, 256);
//        regular_test(524288*2, 256);
          regular_test(8192, 65536);
//        cout<<"Get,Put,Delete thruput"<<endl;
//        uint64_t keygroup[7]={2048,4096,8192,16384,32768,65536,131072};
//        uint64_t valuelength[7]={256,512,1024,2048,4096,8192,16384};
//        cout<<"all key:1024,2048,4096,8192,16384,32768,65536,131072"<<endl<<"  all value :256,512,1024,2048,4096,8192,16384,32768"<<endl;
//        cout<<"keep value the same"<<endl;
//        for(int i=0;i<7;i++){
//            cout<<"key:"<<keygroup[i]<<"value:"<<valuelength[2]<<endl;
//            regular_test(keygroup[i],valuelength[2]);
//        }
        cout<<"keep keynum the same"<<endl;
//        for(int i=0;i<7;i++){
//            cout<<"key:"<<keygroup[5]<<"value:"<<valuelength[i]<<endl;
//            regular_test(keygroup[5],valuelength[i]);
//        }
//        cout<<"compute compaction effect"<<endl;
//        regular_test(524288, 1024);
//        int size = 1024, max = 1024;
//        fout.open("output-all-3", std::ios::out);
//        for (int i = 1; i < 11; ++i) {
//            for (int j = 0; j < 7; ++j) {
//                random_test(max*4*i, size*64);
//            }
//        }
//        fout.close();
//        fout.open("output-all-3", std::ios::out);
//        random_test(65536, 24576);
//        random_test(65536, 36864);
//        random_test(1024*(1<<0), 1024*(1<<6));
//        for (int i = 0; i < 8; ++i) {
//            random_test(1024*(1<<i), 1024*1024/(1<<i));
//        }


    }
};

int main(int argc, char *argv[])
{
    bool verbose = (argc == 2 && std::string(argv[1]) == "-v");

    std::cout << "Usage: " << argv[0] << " [-v]" << std::endl;
    std::cout << "  -v: print extra info for failed tests [currently ";
    std::cout << (verbose ? "ON" : "OFF")<< "]" << std::endl;
    std::cout << std::endl;
    std::cout.flush();
    PerformanceTest test("./data", verbose);

    test.start_test();

    return 0;
}
