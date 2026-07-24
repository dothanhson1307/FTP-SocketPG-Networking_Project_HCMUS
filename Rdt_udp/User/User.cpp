#include "User.h"

User::User(string n,string p):username(n),password(p){}

string User::getUsername(){return username;}

string User::getPassword(){return password;}