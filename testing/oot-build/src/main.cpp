#include <memory>

#include "HelloWorld.hpp"

int main(int, char*[])
{
    auto pHelloWorld = std::make_shared<CHelloWorld>();
    pHelloWorld->HelloWorld();

    CHelloWorld helloWorld;
    helloWorld.HelloWorld();

    return 0;
}
