#include "fileUtilities.h"

string getBaseName(const string &directory){
    //findlastof right->left
    size_t slash_pos = directory.find_last_of('/');
    if(slash_pos != string::npos){
        return directory.substr(slash_pos+1);
    }
    return directory;
}