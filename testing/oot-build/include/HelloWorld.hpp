#pragma once

class CHelloWorld
{
public:
    CHelloWorld();
    ~CHelloWorld() = default;

    bool HelloWorld();

private:
    int mSecret;
};
