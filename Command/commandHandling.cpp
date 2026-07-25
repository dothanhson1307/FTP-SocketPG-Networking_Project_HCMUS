#include "commandHandling.h"
#include "command.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>
#include <cctype>
#include <unordered_map>

bool read(const string& s, std::vector<std::vector<string>>& args) {
    std::stringstream ss(s);
    string line;
    while(getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if(!line.empty()) {
            std::vector<string> v;
            std::stringstream ss_1(line);
            string tmp;
            
            // Convert all command at index 0 to uppercase
            if (!(ss_1 >> tmp)) {
                continue;
            }

            // toupper chi nhan unsigned, toupper convert qua int nen phai ep kieu ve char
            for (char& c : tmp) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            v.push_back(tmp);
            
            while(ss_1 >> tmp) {
                if(!tmp.empty())
                    v.push_back(tmp);
            }
            args.push_back(v);
        }
    }
    return true;
}

std::unordered_map<string, CommandHandler> router = {
    {"USER", handleUser},
    {"PASS", handlePass},
    {"QUIT", handleQuit}
};

string executeCommand(const std::vector<string>& args, SessionState& session) {
    if (args.empty()) {
        return "500 Syntax error, command unrecognized!\r\n";
    }

    auto iterator = router.find(args[0]);

    if (iterator == router.end()) {
        return "502 Command not implemented!\r\n";
    }
    
    // function call
    return iterator->second(args, session);
}