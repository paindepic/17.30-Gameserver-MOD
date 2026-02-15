#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <algorithm>  
#include <random>  
#include <vector>      
#include <regex>      
#include <sstream>      
#include <thread>      
#include <filesystem>

#include "../SDK/SDK.hpp"
using namespace SDK;

#include "Vendor/MinHook/MinHook.h"
#pragma comment(lib, "Vendor/MinHook/MinHook.lib")

#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)
#define PI 			    (3.1415926535897932f)
