#include <windows.h>

#include <sql.h>
#include <sqlext.h>

#include <stdio.h>
#include <iostream>
#include <unordered_map>


#include <SQLConnection.h>
#include <MemoryCheckpoint.h>
#include <HeapCheck.h> 
#include <stdfloat>

//
// In this file is some test to check the usage and if works an 
// SQLConnection library 
// 



int main() {
    HEAPCHECK;
    MEMORYCHECKPOINT;

    
    mSQL::SQLConnection& sql = mSQL::SQLConnection::getSQLConnection();

    auto* map = sql.getAllUsersCredentials();
    for (auto it = map->begin(); it != map->end(); ++it) {
        std::cout << "Account: " << (*it).first << " Password: " << (*it).second << "\n";
    }
    
    delete map;

    std::string lg = "56543@student.uni.opole.pl";
    std::string ps = "140083";
    std::cout << sql.Login(lg, ps) << "\n";


    std::cout << sql.Register(lg, ps) << "\n";

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    //sql.SendMessageToDB("140083@student.uni.opole.pl", "HopeAndBelieve@mailfence.com", now, "And dreadfully distinct against the dark");
    
    std::cout << sql.MailExist("134234@student.uni.opole.pl") << "\n";
    
    auto* res = sql.RetMessaffeFromDbReceived("140083@student.uni.opole.pl", 0);

    for (auto it = res->begin(); it != res->end(); ++it) {
        auto tup = *it;
        
        std::cout << "Rcp: " << std::get<0>(tup) << "\n" <<
            "Sndr: " << std::get<1>(tup) << "\n" <<
            "Date: " << std::get<2>(tup) << "\n" <<
            "Msg: " << std::get<3>(tup) << "\n";
    }
    delete res;

    auto* res2 = sql.RetMessaffeFromDbSend("HopeAndBelieve@mailfence.com", 0);
    std::cout << "\n====================\n";
    for (auto it = res2->begin(); it != res2->end(); ++it) {
        auto tup = *it;

        std::cout << "Rcp: " << std::get<0>(tup) << "\n" <<
            "Sndr: " << std::get<1>(tup) << "\n" <<
            "Date: " << std::get<2>(tup) << "\n" <<
            "Msg: " << std::get<3>(tup) << "\n";
    }
    delete res2;
    mSQL::SQLConnection::destroySingleton();
    return 0;
}
