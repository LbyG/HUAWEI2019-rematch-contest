#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

using namespace std;

class config {
public:
    int priority_N = 2;
    int max_T = 100000;
    int max_channel = 100;
    int max_path_regulation_fail_count = 3000;
    double congestion_threshold = 0.0;
    // if deadlock car not in this epoch schedule, then use 1/metric ratio to abondon
    int prevent_deadlock_metric = 3;
    // channel for road capacity is channel_metric^channel
    double channel_metric = 0.6;
    // road capacity metric
    double capacity_metric = 0.9;
    vector<double> channel_weight;
    double count_capacity(int channel, int length, int speed);
    config();
};

#endif