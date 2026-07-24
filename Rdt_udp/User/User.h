#pragma once
#include <string>
#include <vector>

using std::string;
using std::vector;

class User{
    private:
        string username;
        string password;
    public:
        User(string,string);
        string getUsername();
        string getPassword();
};