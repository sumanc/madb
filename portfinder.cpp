//
//  portfinder.cpp
//  madb
//
//  Created by Suman Cherukuri on 8/6/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#include "portfinder.hpp"
#include <fcntl.h>
#include <sys/file.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

void reportErrorAndExit(const char* msg) {
    perror(msg);
    exit(-1);
}

bool isLocalPortFree(int port) {
    std::string hostname = std::string("127.0.0.1");
    
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return true;
    }
    
    server = gethostbyname(hostname.c_str());
    
    if (server == NULL) {
        return false;
    }
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
           (char *)server->h_addr,
          server->h_length);
    
    bool ret = true;
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        ret = true;
    }
    else {
        ret = false;;
    }
    close(sockfd);
    return ret;
}

int getFreePort(std::unordered_map<int, std::string> p2dMap) {
    //get a port between 6000 and 7000
    int port = 0;
    for (int i = 6000; i < 7000; i++) {
        std::string d = p2dMap[i];
        if (d != "") {
            continue;
        }
        bool isFree = isLocalPortFree(i);
        if (isFree) {
            port = i;
            break;
        }
    }
    
    return port;
}

void releasePort(int port) {

}

void getDeviceMapping(std::unordered_map<int, std::string>& p2dMap, std::unordered_map<std::string, int>& d2pMap) {
    FILE *f = fopen("/var/mesmer/deviceport.mesmer", "a+");
#if __linux__
    while (flock(fileno(f), LOCK_EX) == -1) {
        if (errno == EWOULDBLOCK) {
            sleep(1000);
            continue;
        }
        else {
            reportErrorAndExit("flock");
        }
    }
#endif
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *contents = (char *)malloc(fsize + 1);
    int ret = (int)fread(contents, 1, fsize, f);
    if (ret == -1) {
        reportErrorAndExit("fread");
    }
    contents[fsize] = 0;
    std::string devicesStr = std::string(contents);
    
    /*
     // androind on ubuntu doesn't compile for shared mem
     int fd = shm_open("deviceport.mesmer",
     O_RDWR | O_CREAT, // | O_EXLOCK,
     0666);
     if (fd < 0) {
     reportErrorAndExit("Can't open shared mem segment...");
     }
     
     #if __linux__
     while (flock(fd, LOCK_EX) == -1) {
     if (errno == EWOULDBLOCK) {
     sleep(1000);
     continue;
     }
     else {
     reportErrorAndExit("flock");
     }
     }
     #endif
     
     int size = 4096;
     
     int rtrn = ftruncate(fd, size);
     if (rtrn == -1) {
     perror("ftruncate");
     }
     caddr_t memptr = (caddr_t)mmap(NULL,
     size,
     PROT_READ | PROT_WRITE,
     MAP_SHARED,
     fd,
     0);
     
     if ((caddr_t) -1  == memptr) {
     reportErrorAndExit("Can't get segment...");
     }
     std::string devicesStr = std::string(memptr);
     */
    int port = 0;
    std::string line;
    std::stringstream devicesStream(devicesStr);
    while(std::getline(devicesStream, line, ',')) {
        std::size_t found = line.find(':');
        if (found != std::string::npos) {
            std::string portStr = line.substr(found + 1);
            port = std::stoi(portStr);
            std::string device = line.substr(0, found);
            d2pMap[device] = port;
            p2dMap[port] = device;
        }
    }
#if __linux__
    flock(fileno(f), LOCK_UN);
#endif
    //    adb_close(fd);
    fclose(f);
}

int getDevicePort(std::string serial) {
    std::unordered_map<std::string, int> d2pMap;
    std::unordered_map<int, std::string> p2dMap;
    getDeviceMapping(p2dMap, d2pMap);
    
    int port = d2pMap[serial];
    if (port == 0) {
        // find an open port and update shared memory
        port = getFreePort(p2dMap);
        std::stringstream ss;
        ss << ":" << port << ",";
        serial += ss.str();
//        strncat(memptr, serial.c_str(), serial.length());
        FILE *f = fopen("/var/mesmer/deviceport.mesmer", "a+");
#if __linux__
        while (flock(fileno(f), LOCK_EX) == -1) {
            if (errno == EWOULDBLOCK) {
                sleep(1000);
                continue;
            }
            else {
                reportErrorAndExit("flock");
            }
        }
#endif
        int ret = fprintf(f, "%s", serial.c_str());
        if (ret == -1) {
            perror("fprintf");
        }
#if __linux__
        flock(fileno(f), LOCK_UN);
#endif
        fclose(f);
    }
    
//    munmap(memptr, size);
//    adb_close(fd);
    return port;
}

//int getDevicePort(std::string serial) {
//    int port = -1;
//    key_t key = ftok("mesemer_devices", 65);
//    int shmid = shmget(key, 4096, 0666|IPC_CREAT);
//#if __linux__
//    int rtrn = shmctl(shmid, SHM_LOCK, (struct shmid_ds *) NULL);
//#endif
//    char *devices = (char*) shmat(shmid,(void*)0, 0);
//    std::string devicesStr = std::string(devices);
//
//    std::unordered_map<std::string, int> d2pMap;
//    std::unordered_map<int, std::string> p2dMap;
//
//    std::string line;
//    std::stringstream devicesStream(devicesStr);
//    while(std::getline(devicesStream, line, ',')) {
//        std::size_t found = line.find(':');
//        if (found != std::string::npos) {
//            std::string portStr = line.substr(found + 1);
//            port = std::stoi(portStr);
//            std::string device = line.substr(0, found);
//            d2pMap[device] = port;
//            p2dMap[port] = device;
//        }
//    }
//
//    port = d2pMap[serial];
//    if (port == 0) {
//        // find an open port and update shared memory
//        port = getFreePort(p2dMap);
//        std::stringstream ss;
//        ss << ":" << port << ",";
//        serial += ss.str();
//        strncat(devices, serial.c_str(), serial.length());
//    }
//    else {
//        std::string line;
//        std::stringstream devicesStream(devicesStr);
//        while(std::getline(devicesStream, line, ',')) {
//            if (line.find(serial) != std::string::npos) {
//                std::size_t found = line.find(':');
//                if (found != std::string::npos) {
//                    std::string portStr = line.substr(found + 1);
//                    port = std::stoi(portStr);
//                    break;
//                }
//            }
//        }
//    }
//
//    shmdt(devices);
//#if __linux__
//    rtrn = shmctl(shmid, SHM_UNLOCK, (struct shmid_ds *) NULL);
//#endif
//    // to destroy
//    //    shmctl(shmid, IPC_RMID, NULL);
//    return port;
//}
