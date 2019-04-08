#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

using namespace std;

class config {
public:
    int priority_N = 2;
    int max_T = 100000;
    int max_channel = 100;
    vector<double> channel_weight;
    double count_capacity(int channel, int length, int speed);
    config();
};

#endif