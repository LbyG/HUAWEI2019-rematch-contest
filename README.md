# HUWEI2019-rematch-contest
HUWEI2019 rematch contest

TODO<br>
* (效果不太好)修改解死锁的方法，如果死锁道路上无新安排车辆则撤回会通过死锁道路的车辆

4.11号<br>
* 修改了一个巨大的bug，之前对于规划车辆的发车时间设置错误！！！！
* 修改对于车辆通过道路时间的预估，使之更接近于实际情况的估计
* 使用road::count_situation_car_wait_into_road加速road::get_through_time中对start_time中ratio的计算
* 线上判题器对于空间的限制大概为1G，当使用空间过大的时候会program run fail

4.9号<br>
* 新的调度模型，先优先车辆后普通车辆，预估距离长的车辆优先考虑出发。
* 一个个时间段的进行车辆的路径规划，每个时间段路径规划后使用调度器检查是否有死锁。如果死锁则撤回路口死锁的新安排车辆（car.whether_finish_find_path==1），反复撤回，直到没有死锁为止。如果路口死锁车辆都是预置车辆（car.whether_preset==1）和已安排车辆（car.whether_finish_find_path==2）,则以1/prevent_deadlock_metric的概率撤回还在路上的新安排车辆（car.whether_finish_find_path==1）
* 通过对调度器状态的save和load，加速调度器检测死锁。如果当下安排[start_time,end_time]的发车车辆，那么[0, start_time-1]之间的车辆调度是固定的。将新安排的车辆加入车辆调度，然后检查死锁和接触死锁（撤回新安排的车辆）

4.10号<br>
* 修改了解锁的方法。当发生死锁时，撤回死锁路上的等待状态（car.schedule_status==1）的新安排车辆（car.whether_finish_find_path==1），每条道路最多撤回config.max_withdraw_in_road辆车。
* 修改了道路能否通行的判断方法。如果在车辆通过道路的某个时刻的car_num/capacity>config.max_car_capacity_ratio，则车辆无法通过该道路。如果车辆通过道路的总时间里car_num_sum/capacity_sum>config.max_car_capacity_sum_ratio，则车辆无法通过道路。
* 加速对于车辆的路径规划。如果前一辆车无法在time时刻从from到to，那么下一辆车也大概率无法在time时刻从from到to。主要通过overall_schedule.car_can_from_to, car_can_from_to_flag和car_can_from_to_time来维护。如果car_can_from_to[car_can_from_to_time][from][to] == car_can_from_to_flag[car_can_from_to_time][from]则表示车可以在car_can_from_to_time从from到to
