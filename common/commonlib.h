#ifndef COMMONLIB_H
#define COMMONLIB_H
#include <iostream>
#include <json/json.h>
#include <Eigen/Dense>
#include <assert.h>
#include <memory>

using namespace Json;
using namespace Eigen;
using namespace std;

typedef Json::Value JsonValue;
typedef Json::Reader JsonReader;
using JsonWriter = Json::Writer;

#endif