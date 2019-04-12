#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

using namespace std;

class config {
public:
    //into_capacity = into_capacity_metric * get_channel_weight(channel) * speed;
    double into_capacity_metric = 0.5;
    
    int MAX_VAL = 1e8;
    // wait_into_road_direction_count[priority_N][direct_N - 1], situation_car_into_road[max_T][priority_N][direct_N - 1]
    int priority_N = 2;
    // wait_into_road_direction_count[priority_N][direct_N - 1], situation_car_into_road[max_T][priority_N][direct_N - 1]
    int direct_N = 4;
    // situation_car_running_in_road[max_T], situation_car_into_road[max_T][priority_N][direct_N - 1] 
    int max_T = 500;
    int max_channel = 100;
    int max_path_regulation_fail_count = 200000;
    // if car number / capacity > max_car_capacity_ratio in one time, then this road can't through
    double max_car_capacity_ratio = 1.0;
    // if sum car number / sum capacity > max_car_capacity_sum_ratio, then this road can't through
    double max_car_capacity_sum_ratio = 1.0;
    // road length = congestion_metric * (1 + max(0, congestion - congestion - threshold))
    double congestion_threshold = 0.0;
    double congestion_metric = 0.8;
    // deadlock only withdraw max_withdraw_in_road car in a road
    int max_withdraw_in_road = 10;
    int max_withdraw_by_random = 100;
    // if deadlock car not in this epoch schedule, then use 1/metric ratio to abondon
    int prevent_deadlock_metric = 10;
    // channel for road capacity is channel_metric^channel
    double channel_metric = 0.8;
    // road capacity metric
    double capacity_metric = 1.0;
    vector<double> channel_weight;
    double count_capacity(int channel, int length, int speed);
    double get_channel_weight(int channel);
    config();
};

#endif