#include "flightlib/envs/quadrotor_env/quadrotor_env_bydata_traj.hpp"

namespace flightlib {

QuadrotorEnvByDataTraj::QuadrotorEnvByDataTraj()
  : QuadrotorEnvByDataTraj(getenv("FLIGHTMARE_PATH") +
                 std::string("/flightlib/configs/quadrotor_env.yaml")) {}

std::string trajPath1 = "/home/avidavid/Downloads/CPC16_Z1.csv";
std::string trajPath2 = "/home/avidavid/Downloads/CPC25_Z1 (1).csv";
std::string trajPath3 = "/home/avidavid/Downloads/CPC33_Z1 (1).csv";
std::string trajPath4 = "/home/avidavid/Downloads/random_states.csv";
std::string trajPath5 = "/home/avidavid/Downloads/random_states (1).csv";
std::string trajPath6 = "/home/avidavid/Downloads/0.016.csv";
std::string trajPath7 = "/home/avidavid/Downloads/data01.csv";
std::string trajPath8 = "/home/avidavid/Downloads/3m_vel_norm.csv";
std::string trajPath9 = "/home/avidavid/Downloads/6m_vel_norm.csv";

std::string cirPath1 = "/home/avidavid/Downloads/4m_circle.csv";
std::string cirPath2 = "/home/avidavid/Downloads/6m_circle.csv";
std::string cirPath3 = "/home/avidavid/Downloads/8m_circle.csv";


// Store second last state and use it for computing bell curve rewards at terminal state
// Vector<quadenv::kNObs> second_last_state;


QuadrotorEnvByDataTraj::QuadrotorEnvByDataTraj(const std::string &cfg_path)
  : EnvBase(),
    pos_coeff_(0.0),
    ori_coeff_(0.0),
    lin_vel_coeff_(0.0),
    ang_vel_coeff_(0.0),
    act_coeff_(0.0),
    goal_state_((Vector<quadenv::kNObs>() << 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0,
                 0.0, 0.0, 0.0, 0.0, 0.0)
                  .finished()) {
  // load configuration file
  YAML::Node cfg_ = YAML::LoadFile(cfg_path);

  std::random_device rd;
  random_gen_ = std::mt19937(rd());

  quadrotor_ptr_ = std::make_shared<Quadrotor>();
  // update dynamics
  QuadrotorDynamics dynamics;
  dynamics.updateParams(cfg_);
  quadrotor_ptr_->updateDynamics(dynamics);

  // define a bounding box
  world_box_ << -50, 50, -50, 50, -50, 50;
  if (!quadrotor_ptr_->setWorldBox(world_box_)) {
    logger_.error("cannot set wolrd box");
  };

  // define input and output dimension for the environment
  // obs_dim_ = quadenv::kNObs * 2;

  // // NEW APPROACH: Let us just focus on relative positions
  obs_dim_ = (quadenv::kNObs * 2) - 3;
  

  act_dim_ = quadenv::kNAct;

  mid_train_step_ = 0;

  Scalar mass = quadrotor_ptr_->getMass();
  act_mean_ = Vector<quadenv::kNAct>::Ones() * (-mass * Gz) / 4;
  act_std_ = Vector<quadenv::kNAct>::Ones() * (-mass * 2 * Gz) / 4;

  // load parameters
  loadParam(cfg_);
}

QuadrotorEnvByDataTraj::~QuadrotorEnvByDataTraj() {}



bool QuadrotorEnvByDataTraj::reset(Ref<Vector<>> obs, const bool random) {
  quad_state_.setZero();
  quad_act_.setZero();
  mid_train_step_ = 0;
  traj_.clear();

  if (random) {
    // randomly reset the quadrotor state
    quad_state_.setZero();
    quad_state_.x(QS::POSZ) = uniform_dist_(random_gen_) + 10;

    float time_partition_window = 1.0f;
    // Pick a random number to choose which data file to use
    // std::uniform_int_distribution<int> data_file_dist(1, 2);
    // int data_file_choice = data_file_dist(random_gen_);
    // Pick traj_path randomly between 8 and 9
    // std::uniform_int_distribution<int> data_file_dist(8, 9);
    // int data_file_choice = data_file_dist(random_gen_);
    // std::cout << "Data file choice: " << data_file_choice << std::endl;


    // // Pick a random circle path from 1 to 3
    // std::uniform_int_distribution<int> data_file_dist(1, 3);
    // int data_file_choice = data_file_dist(random_gen_);

    std::string trajPath;
    // if (data_file_choice == 1){
    //   trajPath = cirPath1;
    // }
    // else if (data_file_choice == 2){
    //   trajPath = cirPath2;
    // }
    // else{
    //   trajPath = cirPath3;
    // }

    trajPath = "/home/avidavid/Downloads/big_circle.csv";

    // if (data_file_choice == 1){
    //   trajPath = trajPath1;
    // }
    // else if (data_file_choice == 2){
    //   trajPath = trajPath2;
    // }
    // else if (data_file_choice == 5){
    //   trajPath = trajPath5;
    // }
    // else if (data_file_choice == 6) {
    //   trajPath = trajPath6;
    // }
    // else if (data_file_choice == 7) {
    //   trajPath = trajPath7;
    // }
    // else if (data_file_choice == 8) {
    //   trajPath = trajPath8;
    // }
    // else if (data_file_choice == 9) {
    //   trajPath = trajPath9;
    // }
    // else{
    //   trajPath = trajPath3;
    // }
    // Using file:
    // std::cout << "Using file: " << trajPath << std::endl;
    std::ifstream dataFile(trajPath);
    std::string line;
    int number_of_lines = 0;
    while (std::getline(dataFile, line)){
      number_of_lines++;
    }
    // printf("Number of lines: %d\n", number_of_lines);

    dataFile.clear();
    dataFile.seekg(0, std::ios::beg);

    std::uniform_int_distribution<int> initial_point(2, number_of_lines-25);

    int initial_point_index = initial_point(random_gen_);
      
    int current_line = 0;
    float initial_time = 0.0f;
    while (std::getline(dataFile, line)){
      current_line++;
      if (current_line == initial_point_index){
        std::istringstream iss(line); // Use string stream
        std::vector<std::string> data;
        std::string token;
        while (std::getline(iss, token, ',')) { // Use ',' as delimiter
          data.push_back(token);
        }
        initial_time = std::stof(data[0]);
        quad_state_.x(QS::POSX) = std::stof(data[1]);
        quad_state_.x(QS::POSY) = std::stof(data[2]);
        quad_state_.x(QS::POSZ) = std::stof(data[3]);
        quad_state_.x(QS::ATTW) = std::stof(data[4]);
        quad_state_.x(QS::ATTX) = std::stof(data[5]);
        quad_state_.x(QS::ATTY) = std::stof(data[6]);
        quad_state_.x(QS::ATTZ) = std::stof(data[7]);
        quad_state_.x(QS::VELX) = std::stof(data[8]);
        quad_state_.x(QS::VELY) = std::stof(data[9]);
        quad_state_.x(QS::VELZ) = std::stof(data[10]);

        // quad_state_.x(QS::OMEX) = std::stof(data[11]);
        // quad_state_.x(QS::OMEY) = std::stof(data[12]);
        // quad_state_.x(QS::OMEZ) = std::stof(data[13]);
        // quad_state_.x(QS::ACCX) = std::stof(data[14]);
        // quad_state_.x(QS::ACCY) = std::stof(data[15]);
        // quad_state_.x(QS::ACCZ) = std::stof(data[16]);
        // std::cout << "Initial Position: " << quad_state_.x.segment<quadenv::kNPos>(quadenv::kPos).transpose() << std::endl;

        // Add a small random noise to the initial state
        quad_state_.x(QS::POSX) += 0.1*uniform_dist_(random_gen_);
        quad_state_.x(QS::POSY) += 0.1*uniform_dist_(random_gen_);
        quad_state_.x(QS::POSZ) += 0.1*uniform_dist_(random_gen_);
        quad_state_.x(QS::VELX) += 0.1*uniform_dist_(random_gen_);
        quad_state_.x(QS::VELY) += 0.1*uniform_dist_(random_gen_);
        quad_state_.x(QS::VELZ) += 0.1*uniform_dist_(random_gen_);
        // quad_state_.qx /= quad_state_.qx.norm();

        break;
      }
    }
    while (std::getline(dataFile, line)) {
      std::istringstream iss(line);
      std::vector<std::string> data;
      std::string token;
      while (std::getline(iss, token, ',')) {
        data.push_back(token);
      }
      if (std::stof(data[0]) - initial_time > time_partition_window){
        // std::cout << "TIMES: " << initial_time << stof(data[0]) << std::endl;
        goal_state_(QS::POSX) = std::stof(data[1]);
        goal_state_(QS::POSY) = std::stof(data[2]);
        goal_state_(QS::POSZ) = std::stof(data[3]);
        goal_state_(QS::ATTW) = std::stof(data[4]);
        goal_state_(QS::ATTX) = std::stof(data[5]);
        goal_state_(QS::ATTY) = std::stof(data[6]);
        goal_state_(QS::ATTZ) = std::stof(data[7]);
        goal_state_(QS::VELX) = std::stof(data[8]);
        goal_state_(QS::VELY) = std::stof(data[9]);
        goal_state_(QS::VELZ) = std::stof(data[10]);

        // goal_state_(QS::OMEX) = std::stof(data[11]);
        // goal_state_(QS::OMEY) = std::stof(data[12]);
        // goal_state_(QS::OMEZ) = std::stof(data[13]);

        break;
      }
      else {
        // Push the state into the trajectory
        Vector<10> state;
        state << std::stof(data[1]), std::stof(data[2]), std::stof(data[3]), std::stof(data[4]), std::stof(data[5]), std::stof(data[6]), std::stof(data[7]), std::stof(data[8]), std::stof(data[9]), std::stof(data[10]);
        traj_.push_back(state);
      }
    }

      // printf("Starting at time: %f\n", initial_time);
      // printf("Starting at position: %f, %f, %f\n", quad_state_.x(QS::POSX), quad_state_.x(QS::POSY), quad_state_.x(QS::POSZ));
      // Check next line for time_partition_window


      // while (std::getline(dataFile, line)){
      //   std::istringstream iss(line);
      //   std::vector<std::string> data;
      //   std::string token;
      //   while (std::getline(iss, token, ',')) {
      //     data.push_back(token);
      //   }
    // int next_point_distance = 0;
    // if (point_to_point)
    //   next_point_distance = 1;
    // else
    //   next_point_distance = 25;

    // // if (std::stof(data[0]) - initial_time > time_partition_window){
    // // If the current line is 15 lines away from the initial point, then we can use this as the goal state
    // for (int i = 0; i < next_point_distance; i++){
    //   std::getline(dataFile, line);
    //   std::istringstream iss(line);
    //   std::vector<std::string> data;
    //   std::string token;
    //   while (std::getline(iss, token, ',')) {
    //     data.push_back(token);
    //   }
    // }

    // Close file if necessary
    dataFile.close();
    // Print Drone's initial position and goal position
    // std::cout << "Goal Position: " << goal_state_.segment<quadenv::kNPos>(quadenv::kPos).transpose() << std::endl;
    // wait 0.1s
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  quadrotor_ptr_->reset(quad_state_);

  cmd_.t = 0.0;
  cmd_.thrusts.setZero();

  getObs(obs);
  return true;
}

bool QuadrotorEnvByDataTraj::resetRange(Ref<Vector<>> obs, int lower_zbound, int upper_zbound, int lower_xybound, int upper_xybound, const bool random) {
  quad_state_.setZero();
  quad_act_.setZero();

  if (random) {
    // randomly reset the quadrotor state
    // reset position
    quad_state_.x(QS::POSX) = uniform_dist_(random_gen_);
    quad_state_.x(QS::POSY) = uniform_dist_(random_gen_);
    quad_state_.x(QS::POSZ) = uniform_dist_(random_gen_) + 5;
    if (quad_state_.x(QS::POSZ) < -0.0)
      quad_state_.x(QS::POSZ) = -quad_state_.x(QS::POSZ);
    // reset linear velocity
    quad_state_.x(QS::VELX) = uniform_dist_(random_gen_);
    quad_state_.x(QS::VELY) = uniform_dist_(random_gen_);
    quad_state_.x(QS::VELZ) = uniform_dist_(random_gen_);
    // reset orientation
    quad_state_.x(QS::ATTW) = uniform_dist_(random_gen_);
    quad_state_.x(QS::ATTX) = uniform_dist_(random_gen_);
    quad_state_.x(QS::ATTY) = uniform_dist_(random_gen_);
    quad_state_.x(QS::ATTZ) = uniform_dist_(random_gen_);
    quad_state_.qx /= quad_state_.qx.norm();
    
    std::uniform_real_distribution<Scalar> altitude_dist(lower_zbound, upper_zbound);
    std::uniform_real_distribution<Scalar> xy_dist(lower_xybound, upper_xybound);

    goal_state_(QS::POSX) = xy_dist(random_gen_);
    goal_state_(QS::POSY) = xy_dist(random_gen_);
    goal_state_(QS::POSZ) = altitude_dist(random_gen_);
  }
  // reset quadrotor with random states
  quadrotor_ptr_->reset(quad_state_);

  // reset control command
  cmd_.t = 0.0;
  cmd_.thrusts.setZero();

  // obtain observations
  getObs(obs);
  return true;
}

// bool QuadrotorEnvByDataTraj::getObs(Ref<Vector<>> obs) {
//   quadrotor_ptr_->getState(&quad_state_);

//   // convert quaternion to euler angle
//   Vector<3> euler_zyx = quad_state_.q().toRotationMatrix().eulerAngles(2, 1, 0);
//   // quaternionToEuler(quad_state_.q(), euler);
//   quad_obs_ << quad_state_.p, euler_zyx, quad_state_.v, quad_state_.w, goal_state_;

//   // obs.segment<quadenv::kNObs>(quadenv::kObs) = quad_obs_;
//   return true;
// }

bool QuadrotorEnvByDataTraj::getObs(Ref<Vector<>> obs) {
  quadrotor_ptr_->getState(&quad_state_);

  // convert quaternion to euler angle
  Vector<3> euler_zyx = quad_state_.q().toRotationMatrix().eulerAngles(2, 1, 0);
  // quaternionToEuler(quad_state_.q(), euler);
  quad_obs_ << quad_state_.p, euler_zyx, quad_state_.v, quad_state_.w;

  // obs.segment<quadenv::kNObs>(quadenv::kObs) = quad_obs_;
  //   obs.segment<quadenv::kNObs>(quadenv::kObs + quadenv::kNObs) = goal_state_;
  // obs(quadenv::kNObs) = goal_state_(QS::POSZ); // add goal state to observation vector


  // NEW APPROACH: Let us just focus on relative positions
  //               This lets us reduce model size and improve performance/generalization speed
  // obs.segment<quadenv::kNObs>(quadenv::kObs) = goal_state_.segment<quadenv::kNObs>(quadenv::kObs) - quad_obs_.segment<quadenv::kNObs>(quadenv::kObs);
  

  // Print current goal state
  // std::cout << "Current Goal State: " << goal_state_(QS::POSX) << ", " << goal_state_(QS::POSY) << ", " << goal_state_(QS::POSZ) << std::endl;
  // Print relative position and velocity and other states
  // std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  // std::cout << "Relative Position: " << obs(0) << ", " << obs(1) << ", " << obs(2) << std::endl;
  // std::cout << "Relative Velocity: " << obs(3) << ", " << obs(4) << ", " << obs(5) << std::endl;
  // std::cout << "Relative Angular Velocity: " << obs(6) << ", " << obs(7) << ", " << obs(8) << std::endl;
  // std::cout << "Relative Orientation: " << obs(9) << ", " << obs(10) << ", " << obs(11) << std::endl;
  // Newest Approach, use relative position then append drone's velocity and orientation followed by goal state velocity and orientation
  obs.segment<quadenv::kNPos>(quadenv::kPos) = goal_state_.segment<quadenv::kNPos>(quadenv::kPos) - quad_obs_.segment<quadenv::kNPos>(quadenv::kPos);
  obs.segment<quadenv::kNOri>(quadenv::kOri) = quad_obs_.segment<quadenv::kNOri>(quadenv::kOri);
  obs.segment<quadenv::kNLinVel>(quadenv::kLinVel) = quad_obs_.segment<quadenv::kNLinVel>(quadenv::kLinVel);
  obs.segment<quadenv::kNAngVel>(quadenv::kAngVel) = quad_obs_.segment<quadenv::kNAngVel>(quadenv::kAngVel);

  obs.segment<quadenv::kNOri>(quadenv::kOri + 9) = goal_state_.segment<quadenv::kNOri>(quadenv::kOri);
  obs.segment<quadenv::kNLinVel>(quadenv::kLinVel + 9) = goal_state_.segment<quadenv::kNLinVel>(quadenv::kLinVel);
  obs.segment<quadenv::kNAngVel>(quadenv::kAngVel + 9) = goal_state_.segment<quadenv::kNAngVel>(quadenv::kAngVel);


  // Print Full Drone State
  // std::cout << quad_obs_.transpose() << std::endl;
  // // Print Full Goal State
  // std::cout << goal_state_.transpose() << std::endl;
  // // Print Full Observation
  // std::cout << obs.transpose() << std::endl;
  // std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  // New ObsDim is 12

  return true;
}

Scalar QuadrotorEnvByDataTraj::step(const Ref<Vector<>> act, Ref<Vector<>> obs) {
  // Print the time and the step
  mid_train_step_++;
  // std::cout << "Taking Step: " << mid_train_step_ << std::endl;
  // std::cout << "Time: " << cmd_.t << ", Step: " << mid_train_step_ << std::endl;
  // // Print out the trajectory
  // for (int i = 0; i < traj_.size(); i++){
  //   std::cout << "Trajectory Point " << i << ": " << traj_[i].transpose() << std::endl;
  // }
  quad_act_ = act.cwiseProduct(act_std_) + act_mean_;
  cmd_.t += sim_dt_;
  cmd_.thrusts = quad_act_;

  // simulate quadrotor
  quadrotor_ptr_->run(cmd_, sim_dt_);

  // update observations
  getObs(obs);

  Matrix<3, 3> rot = quad_state_.q().toRotationMatrix();
  Scalar total_reward = 0.0;

  // New Reward Function, Trajectory Tracking Instead of comparing to goal state
  // print length of traj_
  // std::cout << "Trajectory Length: " << traj_.size() << std::endl;
  int trajectory_length = traj_.size();
  if ((mid_train_step_*2)-1 < trajectory_length){
    int desired_pose_index = mid_train_step_*2 - 1;
    // Print current pose and desired pose
    // std::cout << "Current Pose: " << quad_state_.x.segment<10>(0).transpose() << std::endl;
    // std::cout << "Desired Pose: " << traj_[desired_pose_index].transpose() << std::endl;
    // Pos Reward, Ori Reward, Lin Vel Reward
    // for (int i = 1; i < 10; i++){
    //   // total_reward += (quad_state_(i) - desired_pose(i))*(quad_state_(i) - desired_pose(i)) * pos_coeff_;
    //   total_reward += (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_;
    //   // std::cout << (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_ << std::endl;
    //   // total_reward += 0.01;
    // }
    for (int i = 0; i < 3; i++){
      // total_reward += (quad_state_(i) - desired_pose(i))*(quad_state_(i) - desired_pose(i)) * pos_coeff_;
      total_reward += (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10;
      if (std::isnan((quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10)){
        std::cout << "NAN" << std::endl;
      }
      // std::cout << (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_ << std::endl;
      // total_reward += 0.01;
    }
    // for (int i = 3; i < 7; i++) {
    //   total_reward += (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10;
    //   if (std::isnan((quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10)){
    //     std::cout << "NAN" << std::endl;
    //   }
    //   // std::cout << "Current Orientation: " << i << "is: " << quad_state_.x(i) << std::endl;
    //   // std::cout << "Desired Orientation: " << i << "is: " << traj_[desired_pose_index](i) << std::endl;
    // }
    
    // Get normalized orientation for current state and desired state
    Vector<3> current_orientation;
    current_orientation << quad_state_.x(3), quad_state_.x(4), quad_state_.x(5);
    Vector<3> desired_orientation;
    desired_orientation << traj_[desired_pose_index](3), traj_[desired_pose_index](4), traj_[desired_pose_index](5);
    // Normalize the orientation
    current_orientation /= current_orientation.norm();
    desired_orientation /= desired_orientation.norm();
    // std::cout << "Current Orientation: " << current_orientation.transpose() << std::endl;
    // std::cout << "Desired Orientation: " << desired_orientation.transpose() << std::endl;
    // Apply reward function for orientation
    for (int i = 0; i < 3; i++){
      total_reward += (current_orientation(i) - desired_orientation(i))*(current_orientation(i) - desired_orientation(i)) * pos_coeff_*10;
      if (std::isnan((current_orientation(i) - desired_orientation(i))*(current_orientation(i) - desired_orientation(i)) * pos_coeff_*10)){
        std::cout << "NAN" << std::endl;
      }
    }

    for (int i = 7; i < 10; i++){
      // total_reward += (quad_state_(i) - desired_pose(i))*(quad_state_(i) - desired_pose(i)) * pos_coeff_;
      total_reward += (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10;
      if (std::isnan((quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_*10)){
        std::cout << "NAN" << std::endl;
      }
      // std::cout << (quad_state_.x(i) - traj_[desired_pose_index](i))*(quad_state_.x(i) - traj_[desired_pose_index](i)) * pos_coeff_ << std::endl;
      // total_reward += 0.01;
    }
    // total_reward += (quad_state_.x(0) - traj_[desired_pose_index](0))*(quad_state_.x(0) - traj_[desired_pose_index](0)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(1) - traj_[desired_pose_index](1))*(quad_state_.x(1) - traj_[desired_pose_index](1)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(2) - traj_[desired_pose_index](2))*(quad_state_.x(2) - traj_[desired_pose_index](2)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(3) - traj_[desired_pose_index](3))*(quad_state_.x(3) - traj_[desired_pose_index](3)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(4) - traj_[desired_pose_index](4))*(quad_state_.x(4) - traj_[desired_pose_index](4)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(5) - traj_[desired_pose_index](5))*(quad_state_.x(5) - traj_[desired_pose_index](5)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(6) - traj_[desired_pose_index](6))*(quad_state_.x(6) - traj_[desired_pose_index](6)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(7) - traj_[desired_pose_index](7))*(quad_state_.x(7) - traj_[desired_pose_index](7)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(8) - traj_[desired_pose_index](8))*(quad_state_.x(8) - traj_[desired_pose_index](8)) * pos_coeff_ /10;
    // total_reward += (quad_state_.x(9) - traj_[desired_pose_index](9))*(quad_state_.x(9) - traj_[desired_pose_index](9)) * pos_coeff_ /10;

    // Clamp the reward to be between -5 and 5
    if (total_reward > 3){
      total_reward = 3;
    }
    else if (total_reward < -3){
      total_reward = -3;
    }
    // std::cout << "Total Reward: " << total_reward << std::endl;
    // std::cout << "Current Position X: " << quad_state_.x(0) << std::endl;
    // std::cout << "Desired Position X: " << traj_[desired_pose_index](0) << std::endl;
    // total_reward += (quad_state_x(1) - traj_[desired_pose_index](1))*(quad_state_x(1) - traj_[desired_pose_index](1)) * pos_coeff_;
  // Scalar pos_reward =
  //   pos_coeff_ * (quad_obs_.segment<quadenv::kNPos>(quadenv::kPos) -
  //                 goal_state_.segment<quadenv::kNPos>(quadenv::kPos))
  //                  .squaredNorm();
  // // - orientation tracking
  // Scalar ori_reward =
  //   ori_coeff_ * (quad_obs_.segment<quadenv::kNOri>(quadenv::kOri) -
  //                 goal_state_.segment<quadenv::kNOri>(quadenv::kOri))
  //                  .squaredNorm();
  // // - linear velocity tracking
  // Scalar lin_vel_reward =
  //   lin_vel_coeff_ * (quad_obs_.segment<quadenv::kNLinVel>(quadenv::kLinVel) -
  //                     goal_state_.segment<quadenv::kNLinVel>(quadenv::kLinVel))
  //                      .squaredNorm();

  // // - angular velocity tracking
  // Scalar ang_vel_reward =
  //   ang_vel_coeff_ * (quad_obs_.segment<quadenv::kNAngVel>(quadenv::kAngVel) -
  //                     goal_state_.segment<quadenv::kNAngVel>(quadenv::kAngVel))
  //                      .squaredNorm();
  //   // survival reward
  //   total_reward += 0.3 + pos_reward + ori_reward + lin_vel_reward + ang_vel_reward;
    // total_reward += 0.01;
    // Check if total_reward is NaN
    if (std::isnan(static_cast<double>(total_reward))){
      total_reward = 0;
      std::cout << "Total Reward is NaN" << std::endl;
    }
    if (std::isinf(static_cast<double>(total_reward))){
      total_reward = 0;
      std::cout << "Total Reward is Inf" << std::endl;
    }
    // std::cout << "Total Reward: " << total_reward << std::endl;
    return total_reward;
  }
  else {
    // Punish the drone for not reaching the goal state
    return -0.3;
  }
}

bool QuadrotorEnvByDataTraj::isTerminalState(Scalar &reward) {
  // if ((((quad_state_.x.segment<quadenv::kNPos>(quadenv::kPos) -
  //      goal_state_.segment<quadenv::kNPos>(quadenv::kPos))
  //       .squaredNorm() < 0.1))) {
  //       // return false;
  //   // We want the quadrotor to terminate within 0.1m of the goal, and reward it immediately for doing so
  //   // double dist = (quad_obs_.segment<quadenv::kNPos>(quadenv::kPos) - goal_state_.segment<quadenv::kNPos>(quadenv::kPos)).squaredNorm();
  //   // double power = -0.5*std::pow(dist/0.5, 2);
  //   // reward = 10.0*std::exp(power);
  //   reward = 30.0;
  //   // reward = 0;

  //   // // Use a bell curve to reward the drone for having a velocity that is very close to the desired velocity
  //   // // MAXIMUM REWARD FROM VELOCITY: 40.0, MINIMUM REWARD FROM VELOCITY: 0.0
  //   double vel_dist = (quad_state_.x.segment<quadenv::kNLinVel>(quadenv::kLinVel) - goal_state_.segment<quadenv::kNLinVel>(quadenv::kLinVel)).squaredNorm();
  //   double vel_power = -0.5*std::pow(vel_dist/0.5, 2);
  //   // reward += 40.0*std::exp(vel_power);


  //   // Use a bell curve to reward the drone for having a terminal orientation that is very close to the desired orientation
  //   // MAXIMUM REWARD FROM ORIENTATION: 40.0, MINIMUM REWARD FROM ORIENTATION: 0.0
  //   double ori_dist = (quad_state_.x.segment<quadenv::kNOri>(quadenv::kOri) - goal_state_.segment<quadenv::kNOri>(quadenv::kOri)).squaredNorm();
  //   double ori_power = -0.5*std::pow(ori_dist/0.5, 2);
  //   // reward += 40.0*std::exp(ori_power);
  //   // also print the distances
  //   // Display Drone's velocity and goal velocity
  //   // std::cout << "Drone's Velocity: " << quad_state_.x.segment<quadenv::kNLinVel>(quadenv::kLinVel).transpose() << std::endl;
  //   // std::cout << "Goal Velocity: " << goal_state_.segment<quadenv::kNLinVel>(quadenv::kLinVel).transpose() << std::endl;
  //   std::cout << "Orientation diff: " << ori_dist << std::endl;
  //   // Display Velocity and Orientation Rewards
  //   // std::cout << "Velocity Reward: " << 50.0*std::exp(vel_power) << std::endl;
  //   std::cout << "Velocity diff: " << vel_dist << std::endl;
  //   // std::cout << "Orientation Reward: " << 50.0*std::exp(ori_power) << std::endl;
  //   // std::cout << "Terminal Reward: " << reward << std::endl;
  //   return true;
  // }
  // else if ((quad_state_.x(QS::POSZ) <= -10.0)) {
  //   reward = -10.5;
  //   return true;
  // }
  // Once mid__
  int trajectory_length = traj_.size();
  if ((mid_train_step_*2)-1 >= trajectory_length) {
    // std::cout << "Full Trial Reward" << reward << std::endl;
    // reward = 5;
    return true;
  }
  else {
    reward = 0.0;
    return false;
  }
}

bool QuadrotorEnvByDataTraj::loadParam(const YAML::Node &cfg) {
  if (cfg["quadrotor_env"]) {
    sim_dt_ = cfg["quadrotor_env"]["sim_dt"].as<Scalar>();
    max_t_ = cfg["quadrotor_env"]["max_t"].as<Scalar>();
  } else {
    return false;
  }

  if (cfg["rl"]) {
    // load reinforcement learning related parameters
    pos_coeff_ = cfg["rl"]["pos_coeff"].as<Scalar>();
    ori_coeff_ = cfg["rl"]["ori_coeff"].as<Scalar>();
    lin_vel_coeff_ = cfg["rl"]["lin_vel_coeff"].as<Scalar>();
    ang_vel_coeff_ = cfg["rl"]["ang_vel_coeff"].as<Scalar>();
    act_coeff_ = cfg["rl"]["act_coeff"].as<Scalar>();
  } else {
    return false;
  }
  return true;
}

bool QuadrotorEnvByDataTraj::getAct(Ref<Vector<>> act) const {
  if (cmd_.t >= 0.0 && quad_act_.allFinite()) {
    act = quad_act_;
    return true;
  }
  return false;
}

bool QuadrotorEnvByDataTraj::getAct(Command *const cmd) const {
  if (!cmd_.valid()) return false;
  *cmd = cmd_;
  return true;
}

void QuadrotorEnvByDataTraj::addObjectsToUnity(std::shared_ptr<UnityBridge> bridge) {
  bridge->addQuadrotor(quadrotor_ptr_);
}

std::ostream &operator<<(std::ostream &os, const QuadrotorEnvByDataTraj &quad_env) {
  os.precision(3);
  os << "Quadrotor Environment:\n"
     << "obs dim =            [" << quad_env.obs_dim_ << "]\n"
     << "act dim =            [" << quad_env.act_dim_ << "]\n"
     << "sim dt =             [" << quad_env.sim_dt_ << "]\n"
     << "max_t =              [" << quad_env.max_t_ << "]\n"
     << "act_mean =           [" << quad_env.act_mean_.transpose() << "]\n"
     << "act_std =            [" << quad_env.act_std_.transpose() << "]\n"
     << "obs_mean =           [" << quad_env.obs_mean_.transpose() << "]\n"
     << "obs_std =            [" << quad_env.obs_std_.transpose() << std::endl;
  os.precision();
  return os;
}

}  // namespace flightlib