#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <functional>
#include <vector>
#include <numeric>
#include <future>
#include <filesystem>
#include <cstring>

//g++  -o ./trading_que ./trading_que.cpp  -std=c++14
using pattern_func = std::function<size_t(double *ptr_val, time_t *ptr_timestamps, size_t size)>;

std::vector<std::string> split(std::string str, std::string token)
{
    std::vector<std::string> result;
    while (str.size())
    {
        int index = str.find(token);
        if (index != std::string::npos)
        {
            result.push_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if (str.size() == 0)
                result.push_back(str);
        }
        else
        {
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

struct PriceTimestampSz
{
    PriceTimestampSz(
        const double *p,
        const time_t *t,
        size_t s) : pr(p), tm(t), size(s)
    {
    }
    const double *pr;
    const time_t *tm;
    size_t size = 0;
};

class Distributor
{
public:
    Distributor(const std::vector<double> &v, const std::vector<time_t> &t, size_t windows_sz, size_t step) : prices_(v),
                                                                                                              timestamps_(t),
                                                                                                              window_size_(windows_sz),
                                                                                                              step_(step)
    {
        const auto thread_count = std::thread::hardware_concurrency();
        auto count = v.size() / step;
        std::cout << " count # " << count << "\n";
        size_t scoupe_count = count / thread_count;
        std::cout << " scoupe_count # " << scoupe_count << "\n";
        std::cout << " win " << windows_sz << "\n";
        const double *data = prices_.data();
        const time_t *time = timestamps_.data();

        size_t offset = 0;

        for (size_t i = 0; i < thread_count; ++i)
        {
            PriceTimestampSz pts(
                &data[i * scoupe_count - offset],
                &time[i * scoupe_count - offset],
                v.size() / thread_count + offset);

            std::cout << " offset " << (i * scoupe_count - offset) << "sz =  " << (v.size() / thread_count + offset) << std::endl;
            offsets_.push_back(pts);
            offset = windows_sz - 1;
        }
        offsets_.back().size += v.size() % thread_count;
        std::cout << " offset  + " << v.size() % thread_count << std::endl;
        int thid = 0;
        //for(const auto &v: offsets_) {
        //   std::cout<< "thr = "<<   thid++<<std::endl;
        //   for(size_t i=0;i<v.size;++i) {
        //        std::cout<< " v= "<<  v.pr[i]<<std::endl;
        //   }
        // }
    }

    std::vector<PriceTimestampSz> GetOffsets()
    {

        return offsets_;
    }
    std::vector<PriceTimestampSz> offsets_;
    const std::vector<double> &prices_;
    const std::vector<time_t> &timestamps_;
    const size_t window_size_;
    const size_t step_;
};
int main(int argc, char **argv)
{

    std::cout << " start treading ! \n";

    std::ifstream fprice;

    if (argc > 1)
    {
        std::cout << " ------ FILE " << argv[1] << " ----------\n ";
    }
    else
    {

        return -1;
    }

    std::vector<double> pr_q;

    std::vector<time_t> times;
    fprice.open(argv[1]);
    if (fprice.is_open())
    {

        size_t size = std::filesystem::file_size(argv[1]);
        std::vector<char> data;
        data.resize(size);
        fprice.read(data.data(), size);
         std::cout << " load to memory \n";
        size_t prev_begin = 0;
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (data[i] == '\n')
            {
                size_t sz = (i - prev_begin);
                char str[sz + 1];
                memcpy(str, &data.data()[prev_begin], sz);
                str[sz] = '\0';
                prev_begin = i + 1;

                auto v = split(std::string(str), ";");

                pr_q.push_back(std::stod(v[0]));
                times.push_back(std::stoll(v[1]));
            }
        }
        data.clear();

#if 0
        std::string crs;
        double delta = 0;
        while(!fprice.eof())
        {
            fprice >> crs;
           

            //std::cout<<pr_q.back() <<"  "<< times.back()<<"\n";

        }
#endif
        std::cout << " loaded " << times.size() << "\n";
        size_t window = 900;
        Distributor ds(pr_q, times, window, 4);
        std::vector<PriceTimestampSz> thread_datas = ds.GetOffsets();
        std::cout << " thread_datas " << thread_datas.size() << "\n";

        auto sma = [&](const double *ptr_val, const time_t *ptr_timestamps, size_t size) -> std::vector<double>
        {
            std::vector<double> result;
            result.reserve(size);
            double avg = 0.0;

            for (size_t i = 0; i < size - window + 1; ++i)
            {
                avg = std::accumulate(&ptr_val[i], &ptr_val[i + window], 0.0) / window;

#if 0
                for (size_t v=i;v<i+window;++v) {
                     result.push_back(ptr_val[v] );
                     //std::cout<<i<<"  _"<<ptr_val[v] <<"\n";
                }
               
                //std::cout<<"\n";
                //result.push_back(avg);//max_q / tokenPrice - 1.0
#endif
                result.push_back(avg / ptr_val[i + window - 1] - 1.0); //max_q / tokenPrice - 1.0
            }
            return result;
        };

        std::vector<std::future<std::vector<double>>> ftres;

        int thid = 0;

#if 0
      std::ofstream resma("smares.txt");
      for (const PriceTimestampSz &t:thread_datas) {
            //ftres.push_back(std::async(std::launch::deferred, sma , t.pr, t.tm, t.size));
            //ftres.back().wait();
            //std::cout<<"thid "<<thid++<<"\n";
            //sma(t.pr, t.tm, t.size);
        auto  vec = sma (t.pr, t.tm, t.size);
        for (auto & ft: vec) {
              resma<<ft<<"\n";
           }
         //std::cout<< "_ thr = "<<   thid++<<std::endl;
         // for(size_t i=0;i<t.size;++i) {
         //      std::cout<< " _v= "<<  t.pr[i]<<std::endl;
         // }
        }

#else
        for (const PriceTimestampSz &t : thread_datas)
        {
            ftres.push_back(std::async(std::launch::async, sma, t.pr, t.tm, t.size));
            std::cout << "  newthread \n";
            //ftres.back().wait();
            //std::cout<<"thid "<<thid++<<"\n";
            //sma(t.pr, t.tm, t.size);
        }

        //std::this_thread::sleep_for(std::chrono::seconds(26));
        std::ofstream resma("smares.txt");
        for (int i = 0; i < ftres.size(); ++i)
        {
            std::cout << " thread res save  " << i << "\n";
            auto vec = ftres[i].get();
            for (auto &ft : vec)
            {
                resma << ft << "\n";
            }
        }

#endif

        /*std::ofstream resma("smares.txt");
       for(size_t i=0;i<ftres.size();++i) {
            std::cout<<" thread res save  "<<i<<"\n";
           auto vec = ftres[i].get();
           for (auto & ft: vec) {
              resma<<ft<<"\n";
           }

       }*/
        // for (auto && ft: ftres) {

        //ft.get();
        //  std::cout<<ft.get().size()<<"\n";
        // }

        //std::future<std::vector<double>> res = std::async(std::launch::async, sma , "Data");
    }
}
