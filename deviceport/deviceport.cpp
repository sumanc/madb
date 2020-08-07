//
//  deviceport.cpp
//  deviceport
//
//  Created by Suman Cherukuri on 8/5/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#include <iostream>
#include "portfinder.hpp"

void usage(const char *name) {
    printf("%s: A tool to return a free port for a given device\n\n", name);
    printf("Usage: %s -s <serial number>\n", name);
    
}

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 0;
    }
    if (strncmp(argv[1], "-s", 2) == 0) {
        int port = getDevicePort(std::string(argv[2]));
        printf("%d", port);
        return 1;
    }
    return 0;
}
