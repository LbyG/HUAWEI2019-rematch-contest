#include "overall_schedule.h"

#include "config.h"
#include "util.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

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
    this->all_cars_running_time = vector<int>(config().priority_N, 0);
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
    
    // load cars wait to run in road
    vector<car> cars_tmp;
    for (map<int, car>::iterator iter = this->cars.begin(); iter != this->cars.end(); iter ++)
        cars_tmp.push_back(iter->second);
    sort(cars_tmp.begin(), cars_tmp.end());
    for (list<road>::iterator iter = this->roads_connect_cross.begin(); iter != this->roads_connect_cross.end(); iter ++) {
        iter->clear_cars_wait_to_run_in_road();
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
bool overall_schedule::schedule_cars_one_time_unit() {
    // Schedule cars in road.
    // If car can through cross then car into schedule wait -> car.schedule_status = 1
    // If car blocked by schedule wait car then car into schedule wait -> car.schedule_status = 1
    // If car don't be block and can't through cross then car run one time slice and into end state -> car.schedule_status = 2
    // If car blocked by end state car then car move to the back of the previous car
    this->schedule_cars_running_in_road();
    
    //this->output_schedule_status();
    
    // Cycling the relevant roads at each cross until all car are end state
    while (this->cars_running_wait_state_n > 0) {
    
        int wait_to_termination_n = 0;
        for (map<int, cross>::iterator iter = this->crosses.begin(); iter != this->crosses.end(); iter ++) {
            wait_to_termination_n += iter->second.schedule_cars_in_cross(this->cars_running_n, this->cars_arrive_destination_n, this->all_cars_running_time, this->T, this->arrive_car_id_count, this->arrive_T);
        }
        if (wait_to_termination_n == 0) {
            cout << "cars_running_wait_state_n = " << this->cars_running_wait_state_n << " wait_to_termination_n = " << wait_to_termination_n << endl; 
            this->output_schedule_status();
            return false;
        }
        this->cars_running_wait_state_n -= wait_to_termination_n;
    }
    
    // Schedule cars which arrive schedule time or wait start
    // cars_wait_schedule_start_time_list -> cars_wait_run_list
    // cars_wait_run_list -> cars_in_road
    for (int i = 0; i < config().priority_N; i ++)
        this->schedule_cars_wait_run(i);
    
    //    this->output_schedule_status();
    return true;
}

int overall_schedule::schedule_cars() {
    this->arrive_car_id_count.clear();
    // initial state
    this->initial_cars_state_in_T0();
    // If all cars is arrive then break
    while (!whether_vector_zero(this->cars_wait_schedule_start_time_n) || !whether_vector_zero(this->cars_running_n)) {
        if (!this->schedule_cars_one_time_unit()) {
            cout << "deadblock!!!!!!!!!!!!!!!" << endl;
            return -1;
        }
        //cout << this->T << ":" << this->all_cars_running_time << endl;
        this->T ++;
    }
//    this->output_schedule_status();
    for (int i = 0; i < config().priority_N; i ++)
        cout << "priority = " << i << " all cars running time = " << this->all_cars_running_time[i] << endl;
    return this->T;
}

// output car schedule status
void overall_schedule::output_schedule_status() {
    /*
    cout << "========show schedule status=========" << endl;
    cout << "T = " << this->T << endl;
    
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
    cout << "=====================================" << endl;
    */
}