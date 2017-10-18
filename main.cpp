#include <iostream>
#include <thread>
#include <vector>
#include <mutex>  // std::mutex
#include <atomic> // std::atomic

#include <sstream>
// patch of std::to_string()
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

// Functors:
// http://www.cprogramming.com/tutorial/functors-function-objects-in-c++.html

// http://www.bogotobogo.com/cplusplus/files/CplusplusConcurrencyInAction_PracticalMultithreading.pdf

using namespace std;

std::mutex mu; // for

void shared_print(const std::string& msg)
{
    // now it is synchronized, threads cannot use this resource in the same time
    mu.lock();
    cout << msg << endl;
    mu.unlock();

    // Problem: what if between lock - unlock call, there is an exception, and there is no handling -> mu will be locked forewer...
    // See: With RAII: shared_print_raii() function (resource acquisition is initialization technique)
    // http://en.cppreference.com/w/cpp/language/raii
}

void shared_print_raii(const std::string& msg)
{
    std::lock_guard<std::mutex> guard(mu); // RAII
    cout << msg << endl;
}

void function_3()
{
    cout << "function_3()" << endl;
}

void function_4_naked()
{
    for(unsigned i = 0; i<100; ++i)
        shared_print("function_4_naked(): " + patch::to_string(i));
}

void function_4_raii()
{
    for(unsigned i = 0; i<100; ++i)
        shared_print_raii("function_4_raii(): " + patch::to_string(i));
}

class Functor0
{
public:
    void operator()()
    {
        for(unsigned i = 0; i< 100; ++i)
        {
            cout << "Functor0::operator()(): for(){}:  " << i << endl;
        }
    }

    // without reference parameter
    void operator()(unsigned i)
    {
        for(unsigned j = 0; j< i; ++j)
        {
            cout << "Functor0::operator()(): for(){}:  " << j << endl;
        }

        i = 10;
    }

};
class Functor1
{
public:
    void operator()()
    {
        for(unsigned i = 0; i< 100; ++i)
        {
            cout << "Functor1::operator()(): for(){}:  " << i << endl;
        }
    }

    // with reference parameter, but it is not enough. By default it will be passed by value.
    // use std::ref() to allow it to get param as reference
    void operator()(unsigned& i)
    {
        cout << "Functor1::operator()(): address of parameter: " << &i << endl;
        for(unsigned j = 0; j< i; ++j)
        {
            cout << "Functor::operator()(): for(){}:  " << j << endl;
        }

        i = 10;
    }

};

int main()
{
    cout << "main: MINGW32 version: " << __VERSION__ << endl;
    cout << "main: Hello world!" << endl;

    cout << "indication, how many thread is "<< std::thread::hardware_concurrency() << endl;
    cout << std::this_thread::get_id() << endl;



// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
    // http://en.cppreference.com/w/cpp/language/memory_model
    // Example for 'If a data race occurs, the behavior of the program is undefined.'
    // I.)
{
    int cnt(0);
    auto f = [&]{cnt++;};
    std::thread tu1{f}, tu2{f}, tu3{f}; // undefined behavior

    std::vector<std::thread> tv;

    tv.push_back(std::move(tu1));
    tv.push_back(std::move(tu2));
    tv.push_back(std::move(tu3));

    for(auto& element : tv)
    {
        cout << "main::for-join: " << element.get_id() << endl;
        if(element.joinable()) element.join();
    }
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
    // II.)
{
    std::atomic<int> cnt{0};
    auto f = [&]{cnt++;};
    std::thread t_ok_1{f}, t_ok_2{f}, t_ok_3{f}; // OK

    std::vector<std::thread> tv;

    tv.push_back(std::move(t_ok_1));
    tv.push_back(std::move(t_ok_2));
    tv.push_back(std::move(t_ok_3));

    for(auto& element : tv)
    {
        cout << "main::for-join: " << element.get_id() << endl;
        if(element.joinable()) element.join();
    }
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

{
        // III.) Version: race for std::cout

    unsigned i = 20;

    //1.
    // thread t3(function_3); // t1 starts running

    //2.
    // Functor0 ftr0;
    // thread t3(ftr0); // t1 starts running

    //3.
    // thread t3( (Functor0()) ); // t1 starts running

    //4.
    // thread t3( (Functor0()) ); // t1 starts running

    cout << "main(): address of i: " << &i << endl;

    //5/A -- pass by value by default. missing: std::ref() or std::move()
    // std::thread t3( (Functor1()), i ); // t1 starts running; build error OR passed by value

    // A/B pass by reference
    std::thread t3( (Functor1()), std::ref(i) ); // t1 starts running


    for(unsigned j = 0; j< i; ++j)
    {
        cout << "main(): for(){}:  " << j << endl;
    }

    if(t3.joinable())
        t3.join(); // main thread waits for t1 to finish

    // t1.detach(); // goes to daemon process. do not use it.
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
{

        // IV A.) Version: Synchronized access on resource: std::cout  without RAII

    unsigned i = 100;

    thread t4a( function_4_naked ); // t4 starts running

    for(unsigned j = 0; j< i; ++j)
    {
        shared_print("main(): for(){}: " + patch::to_string(j));
    }

    if(t4a.joinable())
        t4a.join(); // main thread waits for t4a to finish
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
{

        // IV B.) Version: Synchronized access on resource: std::cout  with RAII

    unsigned i = 100;

    thread t4b( function_4_raii ); // t4b starts running

    for(unsigned j = 0; j< i; ++j)
    {
        shared_print("main(): for(){}: " + patch::to_string(j));
    }

    if(t4b.joinable())
        t4b.join(); // main thread waits for t4b to finish
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------


    cout << "main: Bye!" << endl;
    return 0;
}
