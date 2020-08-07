//
//  madb.cpp
//  madb
//
//  Created by Suman Cherukuri on 6/23/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <iostream>
#include <sstream>
#include "portfinder.hpp"

using namespace std;

string runAndGetOutput(string cmd) {
    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1");
    
    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream)) {
            if (fgets(buffer, max_buffer, stream) != NULL) {
                data.append(buffer);
            }
        }
        pclose(stream);
    }
    printf("%s", data.c_str());
    return data;
}

int main(int argc, const char * argv[]) {
    string device = "";
    int port = 0;
    stringstream commandArgs;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-s") {
            if (i + 1 < argc) {
                device = argv[i + 1];
                i++;
                continue;
            }
        }
        if (arg == "-P") {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
                i++;
                continue;
            }
        }

        commandArgs << argv[i] << " ";
    }
    
    if (device == "") {
        if (port == 0) {
            int port = 5037;
            stringstream cmd;
            cmd << "adb -P " << port << " " << commandArgs.str();
            system(cmd.str().c_str());
//            std::size_t found = cmd.str().find("devices");
//            if (found != std::string::npos) {
//                stringstream startCommand;
//                startCommand << "adb -P " << port << " devices";
//                string devicesOut = runAndGetOutput(startCommand.str().c_str());
//                char *pch;
//                pch = strtok((char *)(devicesOut.c_str()), "\n");
//                while (pch != NULL) {
//                    string pchStr = string(pch);
//                    if (pchStr != "List of devices attached" &&
//                        pchStr.rfind("* daemon", 0) != 0) {
//                        cout << pch << endl;
//                    }
//                    pch = strtok(NULL, "\n");
//                }
//            }
            stringstream killCommand;
            killCommand << "adb -P " << port << " kill-server";
            system(killCommand.str().c_str());
        }
        else {
            stringstream cmd;
            cmd << "adb -P " << port << " " << commandArgs.str();
            string out = runAndGetOutput(cmd.str().c_str());
            cout << out << endl;
        }
    }
    else { //if (device.find("emulator-") == std::string::npos) {
        // make sure device is available in this adb server
        int port = getDevicePort(device);
        stringstream devicesCommand;
        devicesCommand << "adb -P " << port << " devices";
        string devices = runAndGetOutput(devicesCommand.str());
        char *pch;
        pch = strtok((char *)(devices.c_str()), "\n");
        while (pch != NULL) {
            if (device == pch) {
                //found the target device
                stringstream cmd;
                cmd << "adb -P " << port << " -s " << device << " " << commandArgs.str();
                system(cmd.str().c_str());
                return 0;
            }
            pch = strtok(NULL, "\n\t");
        }
        
        if (port == 0) {
            cout << "*** No free port found to run adb server ***" << endl;
            return -1;
        }
        
        printf("madb: Default ADB couldn't find the device. Checking with a new adb server at port:%d\n", port);
        stringstream startCommand;
        startCommand << "adb -P " << port << " start-server";
        runAndGetOutput(startCommand.str().c_str());
        
        stringstream runCommand;
        runCommand << "adb -P " << port << " -s " << device << " " << commandArgs.str();
        system(runCommand.str().c_str());
    }
//    else {
//        // pass through for emulators
//    }
    return 0;
}
