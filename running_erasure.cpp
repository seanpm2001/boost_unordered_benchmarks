/* Measuring running erasure times of unordered associative containers
 * without duplicate elements.
 *
 * Copyright 2013-2023 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>

std::chrono::high_resolution_clock::time_point measure_start,measure_pause;

template<typename F>
double measure(F f)
{
  using namespace std::chrono;

  static const int              num_trials=10;
  static const milliseconds     min_time_per_trial(200);
  std::array<double,num_trials> trials;

  for(int i=0;i<num_trials;++i){
    int                               runs=0;
    high_resolution_clock::time_point t2;
    volatile decltype(f())            res; /* to avoid optimizing f() away */

    measure_start=high_resolution_clock::now();
    do{
      res=f();
      ++runs;
      t2=high_resolution_clock::now();
    }while(t2-measure_start<min_time_per_trial);
    trials[i]=duration_cast<duration<double>>(t2-measure_start).count()/runs;
  }

  std::sort(trials.begin(),trials.end());
  return std::accumulate(
    trials.begin()+2,trials.end()-2,0.0)/(trials.size()-4);
}

void pause_timing()
{
  measure_pause=std::chrono::high_resolution_clock::now();
}

void resume_timing()
{
  measure_start+=std::chrono::high_resolution_clock::now()-measure_pause;
}

#include <boost/bind/bind.hpp>
#include <boost/cstdint.hpp>
#include <iostream>
#include <random>
#include <vector>

static std::vector<boost::uint64_t> data,unsuccessful_data;

static inline void initialize_data(unsigned int n)
{
  std::uniform_int_distribution<boost::uint64_t> dist;
  std::mt19937_64                                gen(34862);

  data.clear();
  for(unsigned int i=0;i<n;++i){
    data.push_back(dist(gen));
  }
}

template<typename Container>
Container create(unsigned int n)
{
  Container s;
  for(unsigned int i=0;i<n;++i)s.insert({data[i],i});
  return s;
}

template<typename Container>
struct running_erasure
{
  typedef unsigned int result_type;

  unsigned int operator()(const Container& s0)const
  {
    unsigned int res=0;
    {
      pause_timing();
      Container s=s0;
      resume_timing();
      for(auto first=s.begin(),last=s.end();first!=last;){
        if(first->second%2){
          s.erase(first++);
          ++res;
        }
        else{
          ++first;
        }
      }
      pause_timing();
    }
    resume_timing();
    return res;
  }
};

template<typename T>
boost::reference_wrapper<const T> temp_cref(T&& x)
{
  return boost::cref(static_cast<const T&>(x));
}

template<
  template<typename> class Tester,
  typename Container1,typename Container2,typename Container3,
  typename Container4,typename Container5>
void test(
  const char* title,
  const char* name1,const char* name2,const char* name3,
  const char* name4,const char* name5)
{
  unsigned int n0=10000,n1=10000000,dn=500;
  double       fdn=1.05;

  std::cout<<title<<":"<<std::endl;
  std::cout<<name1<<";"<<name2<<";"<<name3<<";"<<name4<<";"<<name5<<std::endl;

  initialize_data(n1);

  for(unsigned int n=n0;n<=n1;n+=dn,dn=(unsigned int)(dn*fdn)){
    double t;
    unsigned int m=Tester<Container1>()(create<Container1>(n));

    t=measure(boost::bind(
      Tester<Container1>(),
      temp_cref(create<Container1>(n))));
    std::cout<<n<<";"<<(t/m)*10E6;

    t=measure(boost::bind(
      Tester<Container2>(),
      temp_cref(create<Container2>(n))));
    std::cout<<";"<<(t/m)*10E6;
 
    t=measure(boost::bind(
      Tester<Container3>(),
      temp_cref(create<Container3>(n))));
    std::cout<<";"<<(t/m)*10E6;

    t=measure(boost::bind(
      Tester<Container4>(),
      temp_cref(create<Container4>(n))));
    std::cout<<";"<<(t/m)*10E6;

    t=measure(boost::bind(
      Tester<Container5>(),
      temp_cref(create<Container5>(n))));
    std::cout<<";"<<(t/m)*10E6<<std::endl;
  }
}

#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_node_map.hpp>

int main()
{
  using container_t1=absl::flat_hash_map<boost::uint64_t,boost::uint64_t>;
  using container_t2=absl::node_hash_map<boost::uint64_t,boost::uint64_t>;
  using container_t3=boost::unordered_map<boost::uint64_t,boost::uint64_t>;
  using container_t4=boost::unordered_flat_map<boost::uint64_t,boost::uint64_t>;
  using container_t5=boost::unordered_node_map<boost::uint64_t,boost::uint64_t>;

  test<
    running_erasure,
    container_t1,
    container_t2,
    container_t3,
    container_t4,
    container_t5>
  (
    "Running erasure",
    "absl::flat_hash_map",
    "absl::node_hash_map",
    "boost::unordered_map",
    "boost::unordered_flat_map",
    "boost::unordered_node_map"
  );
}

#include "absl/container/internal/raw_hash_set.cc"
#include "absl/hash/internal/hash.cc"
#include "absl/hash/internal/low_level_hash.cc"
#include "absl/hash/internal/city.cc"
