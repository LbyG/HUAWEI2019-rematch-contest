#include "overall_schedule.h"

#include "config.h"
#include "util.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

// return cars number
int overall_schedule::get_cars_n() {
    return this->cars.size();
}

//load cars, roads and crosses info from car_path, road_path and cross_path file
void overall_schedule::load_cars_roads_crosses(string car_path, string road_path, string cross_path) {
    // load cars information from car_path file
    // car_info = (id,from,to,speed,planTime)
    this->cars.clear();
    ifstream cars_info_file(car_path);
    string car_info;
    if (cars_info_file)
    {
        while (getline(cars_info_file, car_info)) {
            if (car_info.size() > 0 && car_info[0] != '#') {
                car new_car = car(car_info);
                this->cars.insert(pair<int, car>(new_car.get_id(), new_car));
                //this->cars[new_car.get_id()] = new_car;
            }
        }
    }
    cars_info_file.close();
    // load roads information from road_path file
    // road_info = (id,length,speed,channel,from,to,isDuplex)
    this->roads.clear();
    ifstream roads_info_file(road_path);
    string road_info;
    if (roads_info_file)
    {
        while (getline(roads_info_file, road_info)) {
            if (road_info.size() > 0 && road_info[0] != '#') {
                road new_road = road(road_info);
                this->roads.insert(pair<int, road>(new_road.get_id(), new_road));
                //this->roads[new_road.get_id()] = new_road;
            }
        }
    }
    roads_info_file.close();
    // load crosses information from cross_path file
    // cross_info = (id,roadId1,roadId2,roadId3,roadId4)
    this->crosses.clear();
    ifstream crosses_info_file(cross_path);
    string cross_info;
    if (crosses_info_file)
    {
        while (getline(crosses_info_file, cross_info)) {
            if (cross_info.size() > 0 && cross_info[0] != '#') {
                cross new_cross = cross(cross_info);
                this->crosses.insert(pair<int, cross>(new_cross.get_id(), new_cross));
                //this->crosses[new_cross.get_id()] = new_cross;
            }
        }
    }
    roads_info_file.close();
    // connect road info and cross info
    // because iter->first is id, so road add to  road_into_cross is from small to large by road_id;
    this->roads_connect_cross.clear();
    for (map<int, road>::iterator iter = this->roads.begin(); iter != this->roads.end(); ++iter) {
        // road from from_id to to_id connect to crosses
        this->roads_connect_cross.push_back(road(iter->second));
        road* from_to_road = &this->roads_connect_cross.back();
        map<int, cross>::iterator cross_iter;
        cross_iter = this->crosses.find(from_to_road->get_to());
        cross_iter->second.add_road_into_cross(from_to_road);
        //this->crosses[from_to_road->get_to()].add_road_into_cross(from_to_road);
        cross_iter = this->crosses.find(from_to_road->get_from());
        cross_iter->second.add_road_departure_cross(iter->first, from_to_road);
        //this->crosses[from_to_road->get_from()].add_road_departure_cross(iter->first, from_to_road);
        // if isDuplex == 1m road from to_id to from_id connect to crosses
        if (iter->second.get_is_duplex() == 1) {
            this->roads_connect_cross.push_back(road(iter->second));
            this->roads_connect_cross.back().swap_from_to();
            road* to_from_road = &this->roads_connect_cross.back();
            cross_iter = this->crosses.find(to_from_road->get_to());
            cross_iter->second.add_road_into_cross(to_from_road);
            //this->crosses[to_from_road->get_to()].add_road_into_cross(to_from_road);
            cross_iter = this->crosses.find(to_from_road->get_from());
            cross_iter->second.add_road_departure_cross(iter->first, to_from_road);
            //this->crosses[to_from_road->get_from()].add_road_departure_cross(iter->first, to_from_road);
        }
    }
    // count metric a and metric b
    this->count_metric_ab();
}

// load cars' schedule plan from answer_path
// schedule_info = [id, schedule_start_time, schedule_path1, schedule_path2, schedule_path3, schedule_path4, ...]
void overall_schedule::load_answer(string answer_path) {
    ifstream answer_info_file(answer_path);
    string answer_info;
    if (answer_info_file)
    {
        while (getline(answer_info_file, answer_info)) {
            if (answer_info.size() > 0 && answer_info[0] != '#') {
                vector<int> answer_vec = parse_string_to_int_vector(answer_info);
                map<int, car>::iterator iter = this->cars.find(answer_vec[0]);
                iter->second.set_schedule_path(answer_vec);
                //cars[answer_vec[0]].set_schedule_path(answer_vec);
            }
        }
    }
    answer_info_file.close();
}

// load preset answer
void overall_schedule::load_preset_answer(string preset_answer_path) {
    ifstream answer_info_file(preset_answer_path);
    string answer_info;
    if (answer_info_file)
    {
        while (getline(answer_info_file, answer_info)) {
            if (answer_info.size() > 0 && answer_info[0] != '#') {
                vector<int> answer_vec = parse_string_to_int_vector(answer_info);
                //this->cars[answer_vec[0]].set_schedule_path(answer_vec);
                map<int, car>::iterator iter = this->cars.find(answer_vec[0]);
                iter->second.set_schedule_path(answer_vec);
            }
        }
    }
    answer_info_file.close();
}

// initial all data in T = 0
void overall_schedule::initial_cars_state_in_T0() {
    this->T = 1;
    this->arrive_T = vector<int>(config().priority_N, 0);
    this->all_cars_running_time = vector<long long>(config().priority_N, 0);
    // number of cars which T < car.schedule_start_time
    this->cars_wait_schedule_start_time_n = vector<int>(config().priority_N, 0);
    // the number of cars which is running in road
    this->cars_running_n = vector<int>(config().priority_N, 0); 
    // the number of cars which is arrive destination
    this->cars_arrive_destination_n = vector<int>(config().priority_N, 0);
    
    // the number of cars wait to through cross or previous car is wait to through cross
    this->cars_running_wait_state_n = 0; 
    // the number of cars was run in this time unit
    this->cars_running_termination_state_n = 0;
    
    // initial road status and load cars wait to run in road
    vector<car> cars_tmp;
    for (map<int, car>::iterator iter = this->cars.begin(); iter != this->cars.end(); iter ++)
        if (iter->second.get_whether_preset() == 1 || iter->second.get_whether_finish_find_path() > 0)
            cars_tmp.push_back(iter->second);
    sort(cars_tmp.begin(), cars_tmp.end());
    for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++) {
        iter->clear_cars_wait_to_run_in_road();
        iter->clear_cars_in_road();
    }
    
    for (vector<car>::iterator iter = cars_tmp.begin(); iter != cars_tmp.end(); iter ++) {
        this->crosses[iter->get_from()].add_car_wait_to_run_in_road(*iter);
        this->cars_wait_schedule_start_time_n[iter->get_priority()] ++;
    }
}

// Schedule cars in road.
// If car can through cross then car into schedule wait -> car.schedule_status = 1
// If car blocked by schedule wait car then car into schedule wait -> car.schedule_status = 1
// If car don't be block and can't through cross then car run one time slice and into end state -> car.schedule_status = 0
// If car blocked by termination state car then car move to the back of the previous car -> car.schedule_status = 0
void overall_schedule::schedule_cars_running_in_road() {
    // schedule all cars' state which running in road
    this->cars_running_wait_state_n = 0;
    for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++) {
        // schedule cars in road
        this->cars_running_wait_state_n += iter->schedule_cars_running_in_road();
    }
    // update roads' state
    for (map<int, cross>::iterator iter = crosses.begin(); iter != crosses.end(); iter ++) {
        // update road state in cross
        iter->second.update_road_state_in_cross();
    }
}

// Schedule cars which arrive schedule time or wait start
// cars_wait_schedule_start_time_list -> cars_wait_run_list
// cars_wait_run_list -> cars_in_road
void overall_schedule::schedule_cars_wait_run(int cars_wait_run_priority) {
    for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++) {
        int cars_wait_run_to_running_N = iter->schedule_cars_wait_run(cars_wait_run_priority, this->T);
        this->cars_wait_schedule_start_time_n[cars_wait_run_priority] -= cars_wait_run_to_running_N;
        this->cars_running_n[cars_wait_run_priority] += cars_wait_run_to_running_N;
    }
}

// schedule cars in one time unit
// if deadblock return false
bool overall_schedule::schedule_cars_one_time_unit(int car_priority) {
    // Schedule cars in road.
    // If car can through cross then car into schedule wait -> car.schedule_status = 1
    // If car blocked by schedule wait car then car into schedule wait -> car.schedule_status = 1
    // If car don't be block and can't through cross then car run one time slice and into end state -> car.schedule_status = 2
    // If car blocked by end state car then car move to the back of the previous car
    this->schedule_cars_running_in_road();
    this->schedule_cars_wait_run(1);
    
    //this->output_schedule_status();
    
    // Cycling the relevant roads at each cross until all car are end state
    while (this->cars_running_wait_state_n > 0) {
    
        int wait_to_termination_n = 0;
        for (map<int, cross>::iterator iter = this->crosses.begin(); iter != this->crosses.end(); iter ++) {
            wait_to_termination_n += iter->second.schedule_cars_in_cross(this->cars_running_n, this->cars_arrive_destination_n, this->all_cars_running_time, this->T, this->arrive_car_id_count, this->arrive_T, this->cars_wait_schedule_start_time_n, this->roads_connect_cross);
        }
        if (wait_to_termination_n == 0) {
            return false;
        }
        this->cars_running_wait_state_n -= wait_to_termination_n;
    }
    
    // Schedule cars which arrive schedule time or wait start
    // cars_wait_schedule_start_time_list -> cars_wait_run_list
    // cars_wait_run_list -> cars_in_road
    for (int i = config().priority_N - 1; i >= 0; i --)
        this->schedule_cars_wait_run(i);
    
    //    this->output_schedule_status();
    return true;
}

int overall_schedule::schedule_cars(int car_priority) {
    this->arrive_car_id_count.clear();
    // initial state
    this->initial_cars_state_in_T0();
    // If all cars is arrive then break
    while (!whether_vector_zero(this->cars_wait_schedule_start_time_n) || !whether_vector_zero(this->cars_running_n)) {
        if (!this->schedule_cars_one_time_unit(car_priority)) {
            cout << "cars_running_wait_state_n = " << this->cars_running_wait_state_n << endl; 
            this->output_schedule_status();
            this->prevent_deadlock(this->T, car_priority);
            return -1;
        }
        //for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++)
        //    iter->count_real_situation_car_running_in_road(this->T);
        this->T ++;
    }
//    this->output_schedule_status();
    
    // merge priority == 0 and priority == 1
    this->all_cars_running_time[0] += this->all_cars_running_time[1];
    this->arrive_T[0] = max(this->arrive_T[0], this->arrive_T[1]);
    // T for priority cars is from early plan time of priority cars to all priority cars arrive destination
    int priority_cars_early_plan_time = 1e8;
    for (map<int, car>::iterator iter = this->cars.begin(); iter != this->cars.end(); iter ++) {
        if (iter->second.get_priority() == 1)
            priority_cars_early_plan_time = min(priority_cars_early_plan_time, iter->second.get_plan_time());
    }
    this->arrive_T[1] -= priority_cars_early_plan_time;
    
    for (int i = 0; i < config().priority_N; i ++)
        cout << "priority = " << i << " T = " << this->arrive_T[i] << " all cars running time = " << this->all_cars_running_time[i] << endl;
    cout << "metric_a = " << this-> metric_a << " metric_b = " << this->metric_b << endl;
    int final_arrive_T = round(this->arrive_T[0] + this->metric_a * this->arrive_T[1]);
    long long final_all_cars_running_time = round(this->all_cars_running_time[0] + this->metric_b * this->all_cars_running_time[1]);
    cout << "final result: " << " T = " << final_arrive_T << " all cars running time = " << final_all_cars_running_time << endl;
    return this->T;
}

// count metric a and metric b
void overall_schedule::count_metric_ab() {
    int min_val = -1e8;
    int max_val = 1e8;
    int cars_N = 0;
    int priority_cars_N = 0;
    int cars_fastest_speed = min_val;
    int cars_lowest_speed = max_val;
    int priority_cars_fastest_speed = min_val;
    int priority_cars_lowest_speed = max_val;
    int cars_later_plan_time = min_val;
    int cars_early_plan_time = max_val;
    int priority_cars_later_plan_time = min_val;
    int priority_cars_early_plan_time = max_val;
    int cars_from_count = 0;
    int priority_cars_from_count = 0;
    int cars_to_count = 0;
    int priority_cars_to_count = 0;
    map<int, int> cars_from_map;
    map<int, int> priority_cars_from_map;
    map<int, int> cars_to_map;
    map<int, int> priority_cars_to_map;
    for (map<int, car>::iterator iter = this->cars.begin(); iter != this->cars.end(); iter ++) {
        car iter_car = iter->second;
        cars_N ++;
        cars_fastest_speed = max(cars_fastest_speed, iter_car.get_speed());
        cars_lowest_speed = min(cars_lowest_speed, iter_car.get_speed());
        cars_later_plan_time = max(cars_later_plan_time, iter_car.get_plan_time());
        cars_early_plan_time = min(cars_early_plan_time, iter_car.get_plan_time());
        cars_from_map[iter_car.get_from()] ++;
        if (cars_from_map[iter_car.get_from()] == 1)
            cars_from_count ++;
        cars_to_map[iter_car.get_to()] ++;
        if (cars_to_map[iter_car.get_to()] == 1)
            cars_to_count ++;
        if (iter_car.get_priority() == 1) {
            priority_cars_N ++;
            priority_cars_fastest_speed = max(priority_cars_fastest_speed, iter_car.get_speed());
            priority_cars_lowest_speed = min(priority_cars_lowest_speed, iter_car.get_speed());
            priority_cars_later_plan_time = max(priority_cars_later_plan_time, iter_car.get_plan_time());
            priority_cars_early_plan_time = min(priority_cars_early_plan_time, iter_car.get_plan_time());
            priority_cars_from_map[iter_car.get_from()] ++;
            if (priority_cars_from_map[iter_car.get_from()] == 1)
                priority_cars_from_count ++;
            priority_cars_to_map[iter_car.get_to()] ++;
            if (priority_cars_to_map[iter_car.get_to()] == 1)
                priority_cars_to_count ++;
        }
    }
    long double metric_N = 1.0 * cars_N / priority_cars_N;
    long double metric_speed = 1.0 * (1.0 * cars_fastest_speed / cars_lowest_speed) / (1.0 * priority_cars_fastest_speed / priority_cars_lowest_speed);
    long double metric_plan_time = 1.0 * (1.0 * cars_later_plan_time / cars_early_plan_time) / (1.0 * priority_cars_later_plan_time / priority_cars_early_plan_time);
    long double metric_from = 1.0 * cars_from_count / priority_cars_from_count;
    long double metric_to = 1.0 * cars_to_count / priority_cars_to_count;
    metric_N = 1.0 * round(metric_N * 1e5) / 1e5;
    metric_speed = 1.0 * round(metric_speed * 1e5) / 1e5;
    metric_plan_time = 1.0 * round(metric_plan_time * 1e5) / 1e5;
    metric_from = 1.0 * round(metric_from * 1e5) / 1e5;
    metric_to = 1.0 * round(metric_to * 1e5) / 1e5;
    this->metric_a = 0.05 * metric_N + 0.2375 * metric_speed + 0.2375 * metric_plan_time + 0.2375 * metric_from + 0.2375 * metric_to;
    this->metric_b = 0.8 * metric_N + 0.05 * metric_speed + 0.05 * metric_plan_time + 0.05 * metric_from + 0.05 * metric_to;
    this->priority_weight = vector<long double>(2, 1);
    this->priority_weight[1] = this->metric_a;
    //this->metric_a = 1.0 * round(this->metric_a * 1e5) / 1e5;
    //this->metric_b = 1.0 * round(this->metric_b * 1e5) / 1e5;
}

// output car schedule status
void overall_schedule::output_schedule_status() {
    
    cout << "========show schedule status=========" << endl;
    cout << "T = " << this->T << endl;
    /*
    cout << "cars_wait_schedule_start_time_n = " << this->cars_wait_schedule_start_time_n << endl;
    cout << "cars_wait_run_n = " << this->cars_wait_run_n << endl;
    cout << "car is list which wait run: ";
    for (vector<car>::iterator iter = this->cars_wait_run_list.begin(); iter != this->cars_wait_run_list.end(); iter ++) {
        cout << iter->get_id() << " ";
    }
    cout << endl;
    cout << "cars_running_n = " << this->cars_running_n << endl;
    cout << "cars_arrive_destination_n = " << this->cars_arrive_destination_n << endl;
    cout << "cars_running_wait_state_n = " << this->cars_running_wait_state_n << endl;
    cout << "cars_running_termination_state_n = " << this->cars_running_termination_state_n << endl;
    cout << "car running in the road:" << endl;   
    for (list<road>::iterator iter = roads_connect_cross.begin(); iter != roads_connect_cross.end(); iter ++) {
     //   if (iter->get_id() == 5074)
     //   if (!iter->if_no_car_through_cross())
            iter->output_status(this->T);
    }
    */
    cout << "=====================================" << endl;
}

//================================================================
    
// count preset car to situation_car_running_in_road
void overall_schedule::preset_car_to_situation(car preset_car) {
    int schedule_start_time = preset_car.get_schedule_start_time();
    int cross_id = preset_car.get_from();
    while (preset_car.get_next_road_in_path() != -1) {
        int next_road_id = preset_car.get_next_road_in_path();
        road* next_road = this->crosses[cross_id].get_road_departure_cross()[next_road_id];
        int speed = min(preset_car.get_speed(), next_road->get_speed());
        int length = next_road->get_length();
        int through_time = (length + speed - 1) / speed;
        next_road->car_running_count(schedule_start_time, schedule_start_time + through_time - 1, this->priority_weight[preset_car.get_priority()]);
        schedule_start_time += through_time;
        cross_id = next_road->get_to();
        preset_car.arrive_next_road_path();
    }
    
}

// some way to prevent deadlock
void overall_schedule::prevent_deadlock(int T, int car_priority) {
    int count = 0;
    for (list<road>::iterator iter = roads_connect_cross.begin(); iter != roads_connect_cross.end(); iter ++) {
        if (!iter->if_no_car_through_cross()) {
            //iter->add_deadlock_situation_car_running_in_road(max(0, T - 100), T + 50, 1);
            car deadlock_car = iter->get_car_priority_through_cross();
            if (deadlock_car.get_whether_preset() == 0) {
                this->deadlock_epoch_step = (this->deadlock_epoch_step) % 9;
                if (deadlock_car.get_priority() == car_priority && deadlock_car.get_whether_finish_find_path() == 1) {
                    this->cars[deadlock_car.get_id()].set_whether_finish_find_path(0);
                    count ++;
                    //this->car_path_regulation(deadlock_car, deadlock_car.get_schedule_start_time() + 1);
                }
            }
            //this->cars[deadlock_car.get_id()].set_schedule_start_time(deadlock_car.get_schedule_start_time() + this->deadlock_step);
        }
    }
    if (count == 0) {
        for (vector<car>::iterator car_iter = this->cars_start_run.begin(); car_iter != this->cars_start_run.end(); car_iter ++) {
            if (this->cars[car_iter->get_id()].get_whether_finish_find_path() == 1) {
                count ++;
                if (count % config().prevent_deadlock_metric == 0)
                    this->cars[car_iter->get_id()].set_whether_finish_find_path(0);
            }
        }
    }
}

// estimate time car go to destination
int overall_schedule::estimate_spend_time(car estimate_car) {
    const int MAX_VAL = 1e8;
    map<int, int> cross_dis;
    map<int, int> cross_flag;
    typedef pair<int, int> QueueItem;
    priority_queue <QueueItem, std::vector<QueueItem>, std::greater<QueueItem> > cross_arrive_time_queue;
    for (map<int, cross>::iterator iter = this->crosses.begin(); iter != this->crosses.end(); iter ++) {
        int cross_id = iter->second.get_id();
        cross_dis[cross_id] = MAX_VAL;
        cross_flag[cross_id] = 0;
    }
    cross_dis[estimate_car.get_from()] = 0;
    cross_arrive_time_queue.push(pair<int, int>(cross_dis[estimate_car.get_from()], estimate_car.get_from()));
    while (true) {
        int update_cross_id = -1;
        int update_dis = MAX_VAL;
        while (!cross_arrive_time_queue.empty()) {
            if (cross_flag[cross_arrive_time_queue.top().second] == 0) {
                update_dis = cross_arrive_time_queue.top().first;
                update_cross_id = cross_arrive_time_queue.top().second;
                cross_arrive_time_queue.pop();
                break;
            }
            cross_arrive_time_queue.pop();
        }
        if (update_cross_id == -1)
            break;
        if (update_cross_id == estimate_car.get_to())
            return update_dis;
        cross_flag[update_cross_id] = 1;
        map<int, road*> road_departure_cross = this->crosses[update_cross_id].get_road_departure_cross();
        for (map<int, road*>::iterator road_iter = road_departure_cross.begin(); road_iter != road_departure_cross.end(); road_iter ++) {
            int to_cross_id = road_iter->second->get_to();
            int speed = min(road_iter->second->get_speed(), estimate_car.get_speed());
            int length = road_iter->second->get_length();
            int through_time = (length + speed - 1) / speed;
            if (cross_dis[to_cross_id] > update_dis + through_time) {
                cross_dis[to_cross_id] = update_dis + through_time;
                cross_arrive_time_queue.push(pair<int, int>(cross_dis[to_cross_id], to_cross_id));
            }
        }
    }
}

// reluate car start time and path
int overall_schedule::car_path_regulation(car car_iter, int start_time) {
    int car_id = car_iter.get_id();
    while (true) {
        map<int, int> cross_arrive_time;
        map<int, bool> updata_flag;
        map<int, int> updata_from;
        map<int, int> updata_road_id;
        typedef pair<int, int> QueueItem;
        priority_queue <QueueItem, std::vector<QueueItem>, std::greater<QueueItem> > cross_arrive_time_queue;
        for (map<int, cross>::iterator cross_iter = this->crosses.begin(); cross_iter != this->crosses.end(); cross_iter ++) {
            cross_arrive_time[cross_iter->first] = 1e6;
            updata_flag[cross_iter->first] = false;
            updata_from[cross_iter->first] = cross_iter->first;
            updata_road_id[cross_iter->first] = 0;
        }
        cross_arrive_time[car_iter.get_from()] = max(car_iter.get_plan_time(), start_time);
        cross_arrive_time_queue.push(pair<int, int>(cross_arrive_time[car_iter.get_from()], car_iter.get_from()));
        while (true) {
            int updata_cross_id = -1;
            int updata_cross_time = 1e6;
            while (!cross_arrive_time_queue.empty()) {
                if (!updata_flag[cross_arrive_time_queue.top().second]) {
                    updata_cross_time = cross_arrive_time_queue.top().first;
                    updata_cross_id = cross_arrive_time_queue.top().second;
                    cross_arrive_time_queue.pop();
                    break;
                }
                cross_arrive_time_queue.pop();
            }
            if (updata_cross_id == -1)
                break;
            updata_flag[updata_cross_id] = true;
            map<int, road*> roads_departure_cross = this->crosses[updata_cross_id].get_road_departure_cross();
            for (map<int, road*>::iterator road_iter = roads_departure_cross.begin(); road_iter != roads_departure_cross.end(); road_iter ++) {
                int speed = min(car_iter.get_speed(), (road_iter->second)->get_speed());
                int length = (road_iter->second)->get_length();
                int through_time = (length + speed - 1) / speed;
                int next_cross_id = (road_iter->second)->get_to();
                double congestion = road_iter->second->check_capacity(updata_cross_time, updata_cross_time + through_time - 1, car_iter.get_speed());
                if (congestion < 1) {
                    congestion = max(congestion - config().congestion_threshold, 0.0);
                    if (cross_arrive_time[next_cross_id] > updata_cross_time + through_time * (1 + congestion)) {
                        cross_arrive_time[next_cross_id] = updata_cross_time + through_time * (1 + congestion);
                        updata_from[next_cross_id] = updata_cross_id;
                        updata_road_id[next_cross_id] = (road_iter->second)->get_id();
                        cross_arrive_time_queue.push(pair<int, int>(cross_arrive_time[next_cross_id], next_cross_id));
                    }
                }
            }
        }
        //cout << cross_arrive_time[car_iter.get_from()] - start_time << " " << car_iter.get_schedule_start_time() << endl;
        if (cross_arrive_time[car_iter.get_to()] == 1e6) { //|| (cross_arrive_time[car_iter.get_from()] - start_time) > car_iter.get_schedule_start_time() * -1.5) {
            return -1;
            //cout << "overall_schedule::car_path_regulation error!!!!!!!!!!!!!!!!!" << endl;
        }
        vector<int> schedule_path;
        schedule_path.clear();
        int cross_id = car_iter.get_to();
        while (cross_id != car_iter.get_from()) {
            int next_cross_arrive_time = cross_arrive_time[cross_id];
            int road_id = updata_road_id[cross_id];
            schedule_path.push_back(road_id);
            cross_id = updata_from[cross_id];
            map<int, road*> roads_departure_cross = this->crosses[cross_id].get_road_departure_cross();
            int now_cross_arrive_time = cross_arrive_time[cross_id];
            roads_departure_cross[road_id]->car_running_count(now_cross_arrive_time, next_cross_arrive_time - 1, this->priority_weight[car_iter.get_priority()]);
        }
        vector<int> schedule_info;
        schedule_info.push_back(car_iter.get_id());
        schedule_info.push_back(cross_arrive_time[car_iter.get_to()]);
        for (vector<int>::reverse_iterator riter = schedule_path.rbegin();riter!=schedule_path.rend();riter++)
            schedule_info.push_back(*riter);
        //car_iter->set_schedule_path(schedule_info);
        this->cars[car_iter.get_id()].set_schedule_path(schedule_info);
        this->cars[car_iter.get_id()].set_whether_finish_find_path(1);
        break;
    }
    return 1;
}

// regulate all cars start time and path
void overall_schedule::cars_path_regulation() {
    int rand_int = 1;
    // init situation car running in_road
    for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++) {
        iter->init_situation_car_running_in_road();
    }
    // Adjust the order of the vehicle from low to high speed anc count preset car to situation_car_running_in_road
    this->cars_to_regulate_path = vector<list<car>>(config().priority_N, list<car>());
    for (map<int, car>::iterator iter = this->cars.begin(); iter != this->cars.end(); iter ++) {
        if (iter->second.get_whether_preset() == 0) {
            car new_car(iter->second);
            //new_car.set_schedule_start_time(-1 * this->estimate_spend_time(new_car));
            new_car.set_schedule_start_time(new_car.get_plan_time());
            this->cars_to_regulate_path[new_car.get_priority()].push_back(new_car);
        } else {
            this->preset_car_to_situation(iter->second);
        }
    }
    for (int car_priority = config().priority_N - 1; car_priority >= 0; car_priority --) {
        int start_time = 1;
        int schedule_step = 1;
        this->cars_to_regulate_path[car_priority].sort();
        this->cars_arrive_schedule_start_time.clear();
        while (true) {
            int wait_schedule_time_car_N = 0;
            wait_schedule_time_car_N += this->cars_to_regulate_path[car_priority].size();
            int wait_run_car_N = cars_arrive_schedule_start_time.size();
            cout << "start_time = " << start_time << " remain_car_N = " << wait_schedule_time_car_N << " " << wait_run_car_N << endl;
            if (wait_schedule_time_car_N + wait_run_car_N == 0)
                break;
            while (!this->cars_to_regulate_path[car_priority].empty() && this->cars_to_regulate_path[car_priority].front().get_schedule_start_time() <= start_time) {
                car new_car(this->cars_to_regulate_path[car_priority].front());
                this->cars_to_regulate_path[car_priority].pop_front();
                new_car.set_schedule_start_time(-1 * this->estimate_spend_time(new_car));
                this->cars_arrive_schedule_start_time.push_back(new_car);
            }
            cout << "start sort" << endl;
            sort(this->cars_arrive_schedule_start_time.begin(), this->cars_arrive_schedule_start_time.end());
            cout << "finish sort" << endl;
            this->cars_start_run.clear();
            this->cars_wait_run.clear();
            int path_regulation_fail_count = 0;
            for (vector<car>::iterator car_iter = this->cars_arrive_schedule_start_time.begin(); car_iter != this->cars_arrive_schedule_start_time.end(); car_iter ++) {
                int flag = -1;
                if (path_regulation_fail_count < config().max_path_regulation_fail_count)
                    for (int i = start_time; i < start_time + schedule_step; i += max(1, schedule_step / 10)) {
                        flag = this->car_path_regulation(*car_iter, start_time);
                        if (flag != -1)
                            break;
                    }
                //cout << start_time << " " << flag << endl;
                if (flag != -1) {
                    // car start run
                    this->cars_start_run.push_back(*car_iter);
                } else {
                    // car wait run
                    this->cars_wait_run.push_back(*car_iter);
                    path_regulation_fail_count ++;
                }
            }
            cout << "before " << this->cars_start_run.size() << " " << this->cars_wait_run.size() << endl;
            while (true) {
                if (this->schedule_cars(car_priority) > 0)
                    break;
                /*
                for (vector<car>::iterator car_iter = this->cars_start_run.begin(); car_iter != this->cars_start_run.end(); car_iter ++)
                    if (this->cars[car_iter->get_id()].get_whether_finish_find_path() == 1) {
                        rand_int = (rand_int + 3) % 10;
                        if (rand_int < 3)
                            this->cars[car_iter->get_id()].set_whether_finish_find_path(0);
                    }
                    */
            }
            cout << "after " << this->cars_start_run.size() << " " << this->cars_wait_run.size() << endl;
            for (vector<car>::iterator car_iter = this->cars_start_run.begin(); car_iter != this->cars_start_run.end(); car_iter ++) {
                if (this->cars[car_iter->get_id()].get_whether_finish_find_path() == 0) {
                    car_iter->set_schedule_start_time(-1 * this->estimate_spend_time(*car_iter));
                    this->cars_wait_run.push_back(*car_iter);
                } else {
                    this->cars[car_iter->get_id()].set_whether_finish_find_path(2);
                }
            }
            this->cars_arrive_schedule_start_time = this->cars_wait_run;
            start_time += schedule_step;
            if (this->cars_arrive_schedule_start_time.size() < 5000)
                schedule_step = 5;
            else if (start_time > 100)
                schedule_step = 50;
            else if (start_time > 10)
                schedule_step = 5;
        }
    }
}
    
// write answer to answer file
void overall_schedule::save_answer(string answer_path) {
    ofstream answer_info_file(answer_path);
    for (map<int, car>::iterator car_iter = this->cars.begin(); car_iter != this->cars.end(); car_iter ++) {
        if (car_iter->second.get_whether_preset() == 0)
            answer_info_file << car_iter->second.to_string() << endl;
    }
    answer_info_file.close();
}