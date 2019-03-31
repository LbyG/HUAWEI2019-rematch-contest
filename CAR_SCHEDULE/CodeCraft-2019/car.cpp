#include "car.h"
#include "util.h"

#include <vector>
#include <iostream>

using namespace std;
/*
car::car() {
    // TODO
    this->id = 0;
}
*/
// car_info = (id,from,to,speed,planTime)
car::car(string car_info) {
    // car_info = (id,from,to,speed,planTime)
    vector<int> info_val = parse_string_to_int_vector(car_info);
    if (info_val.size() != 5) {
        cout << "car::car" << endl;
    }
    this->id = info_val[0];
    this->from = info_val[1];
    this->to = info_val[2];
    this->speed = info_val[3];
    this->plan_time = info_val[4];
    if (this->from == this->to)
        cout << "car::car error !!!!!!!!!!!!!" << this->id << endl;
    
    // schedule_status == 0 termination state; schedule_status == 1 wait state;
    this->schedule_status = 0; 
    // distance from the car to the cross
    this->dis_to_cross = 0;
    // channel id which car running in
    this->channel_id = -1;
}

// schedule_info = [id, schedule_start_time, schedule_path1, schedule_path2, schedule_path3, schedule_path4, ...]
void car::set_schedule_path(vector<int> schedule_info) {
    int len = schedule_info.size();
    if (len > 2) {
        this->schedule_start_time = schedule_info[1];
        this->schedule_path.clear();
        for (int i = 2; i < len; i ++)
            this->schedule_path.push_back(schedule_info[i]);
    }
}

// return car id
int car::get_id() const {
    return this->id;
}

// reutrn car start from cross_id;
int car::get_from() const {
    return this->from;
}

// return car arrive to cross_id;
int car::get_to() const {
    return this->to;
}

// return car speed
int car::get_speed() const {
    return this->speed;
}

// return car plan start time
int car::get_plan_time() const {
    return this->plan_time;
}

void car::set_schedule_start_time(int time) {
    this->schedule_start_time = time;
}

// return car schedule start time
int car::get_schedule_start_time() const {
    return this->schedule_start_time;
}

// return next road_id in path, if path is empty return -1
int car::get_next_road_in_path() const {
    if (this->schedule_path.empty())
        return -1;
    else
        return this->schedule_path.front();
}

// arrive next road, so this->schedule_path.pop_front();
void car::arrive_next_road_path() {
    this->schedule_path.pop_front();
}

// set this -> schedule_status = schedule_status
void car::set_schedule_status(int schedule_status) {
    this->schedule_status = schedule_status;
}

// return car schedule status
int car::get_schedule_status() const {
    return this->schedule_status;
}

// set this -> dis_to_cross = dis_to_cross
void car::set_dis_to_cross(int dis_to_cross) {
    this->dis_to_cross = dis_to_cross;
}

// return distance from the car to the cross
int car::get_dis_to_cross() const {
    return this->dis_to_cross;
}

// set this -> channel_id = channel_id
void car::set_channel_id(int channel_id) {
    this->channel_id = channel_id;
}

// return channel id which car running in
int car::get_channel_id() const {
    return this->channel_id;
}

// Overload < for road by id
bool operator<(const car &a, const car &b) {
    if (a.get_schedule_start_time() == b.get_schedule_start_time())
        return a.get_id() < b.get_id();
    else
        return a.get_schedule_start_time() < b.get_schedule_start_time();
    
}