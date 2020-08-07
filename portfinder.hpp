//
//  portfinder.hpp
//  madb
//
//  Created by Suman Cherukuri on 8/6/20.
//  Copyright © 2020 Suman Cherukuri. All rights reserved.
//

#ifndef portfinder_hpp
#define portfinder_hpp

#include <stdio.h>
#include <iostream>

int getDevicePort(std::string serial);
void releasePort(int port);

#endif /* portfinder_hpp */
