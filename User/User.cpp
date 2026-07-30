#include "User.h"
//#include tao lao

using std::string;

User::User(string n,string p):username(n),password(p){}

string User::getUsername(){return username;}

string User::getPassword(){return password;}

std::vector<User> Accounts = {
    User("son", "son123"),
    User("kiet", "kiet123")
};
