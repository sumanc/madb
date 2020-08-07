//
//  portfinder.cpp
//  madb
//
//  Created by Suman Cherukuri on 8/6/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#include "portfinder.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

void report_and_exit(const char* msg) {
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
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
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
            char fn[10] = {0};
            sprintf(fn, "%d.lock", i);
            int fd = open(fn, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                continue;
            }
            close(fd);
            port = i;
            break;
        }
    }
    
    return port;
}

void releasePort(int port) {
    char fn[10] = {0};
    sprintf(fn, "%d.lock", port);
    remove(fn);
}

int getDevicePort(std::string serial) {
    int fd = shm_open("deviceport",
                      O_RDWR | O_CREAT | O_EXLOCK,
                      0666);
    if (fd < 0) {
        report_and_exit("Can't open shared mem segment...");
    }
    
    int size = 4096;
    
    ftruncate(fd, size); /* get the bytes */
    
    caddr_t memptr = (caddr_t)mmap(NULL,
                                   size,
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED,
                                   fd,
                                   0);
    
    if ((caddr_t) -1  == memptr) {
        report_and_exit("Can't get segment...");
    }
    
    std::string devicesStr = std::string(memptr);
    
    std::unordered_map<std::string, int> d2pMap;
    std::unordered_map<int, std::string> p2dMap;
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
    
    port = d2pMap[serial];
    if (port == 0) {
        // find an open port and update shared memory
        port = getFreePort(p2dMap);
        std::stringstream ss;
        ss << ":" << port << ",";
        serial += ss.str();
        strncat(memptr, serial.c_str(), serial.length());
    }
    else {
        std::string line;
        std::stringstream devicesStream(devicesStr);
        while(std::getline(devicesStream, line, ',')) {
            if (line.find(serial) != std::string::npos) {
                std::size_t found = line.find(':');
                if (found != std::string::npos) {
                    std::string portStr = line.substr(found + 1);
                    port = std::stoi(portStr);
                    break;
                }
            }
        }
    }
    
    munmap(memptr, size);
    close(fd);
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
