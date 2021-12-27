/*
 * Copyright (C) 2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef INCLUDE__FREE_FLEET__MANAGER__MANAGER_HPP
#define INCLUDE__FREE_FLEET__MANAGER__MANAGER_HPP

#include <memory>
#include <vector>
#include <optional>

#include <rmf_utils/impl_ptr.hpp>

#include <rmf_traffic/Time.hpp>
#include <rmf_traffic/agv/Graph.hpp>

#include <free_fleet/Worker.hpp>
#include <free_fleet/manager/RobotInfo.hpp>
#include <free_fleet/manager/CoordinateTransformer.hpp>
#include <free_fleet/transport/ServerMiddleware.hpp>

#include <free_fleet/messages/Waypoint.hpp>
#include <free_fleet/messages/RobotState.hpp>
#include <free_fleet/messages/NavigationRequest.hpp>
#include <free_fleet/messages/RelocalizationRequest.hpp>

namespace free_fleet {

class Manager : public Worker
{
public:

  /// This function returns the current time stamp based on the implementation.
  using TimeNow = std::function<rmf_traffic::Time()>;

  /// This callback function can be used to work with different applications,
  /// triggered every time a robot is updated with an incoming new state.
  using RobotUpdatedCallback =
    std::function<void(const manager::RobotInfo& robot_info)>;

  /// Factory function that creates an instance of the Free Fleet Manager.
  ///
  /// \param[in] fleet_name
  /// \param[in] graph
  /// \param[in] middleware
  /// \param[in] to_robot_transform
  /// \param[in] time_now_fn
  /// \param[in] robot_updated_callback_fn
  /// \return
  static std::shared_ptr<Manager> make(
    const std::string& fleet_name,
    std::shared_ptr<const rmf_traffic::agv::Graph> graph,
    std::unique_ptr<transport::ServerMiddleware> middleware,
    std::shared_ptr<const manager::CoordinateTransformer> to_robot_transform,
    TimeNow time_now_fn,
    RobotUpdatedCallback robot_updated_callback_fn);

  /// Running the operations of the manager once.
  void run_once() override;

  /// Gets all the names of the robots that are currently under this manager.
  ///
  /// \return
  std::vector<std::string> robot_names();

  /// Gets the RobotInfo of the robot with the provided name. If no such robot
  /// exists, a nullptr will be returned.
  ///
  /// \param[in] robot_name
  /// \return
  std::shared_ptr<const manager::RobotInfo> robot(
    const std::string& robot_name);

  /// Gets all the available RobotInfo that has been registered with the manager
  ///
  /// \return
  std::vector<std::shared_ptr<const manager::RobotInfo>> all_robots();

  /// Sends out a pause request to a robot.
  ///
  /// \param[in] robot_name
  ///   Name of the robot that this request is targeted at.
  ///
  /// \return
  ///   Optional of the command ID for this request. Returns a nullopt if there
  ///   does not exist a robot of the provided name.
  std::optional<CommandId> request_pause(const std::string& robot_name);

  /// Sends out a resume request to a robot.
  ///
  /// \param[in] robot_name
  ///   Name of the robot that this request is targeted at.
  ///
  /// \return
  ///   Optional of the command ID for this request. Returns a nullopt if there
  ///   does not exist a robot of the provided name.
  std::optional<CommandId> request_resume(const std::string& robot_name);

  /// Sends out a dock request to a robot.
  ///
  /// \param[in] robot_name
  ///   Name of the robot that this request is targeted at.
  ///
  /// \param[in] dock_name
  ///   Name of the desired dock.
  ///
  /// \return
  ///   Optional of the command ID for this request. Returns a nullopt if there
  ///   does not exist a robot of the provided name.
  std::optional<CommandId> request_dock(
    const std::string& robot_name,
    const std::string& dock_name);

  /// Sends out a relocalization request to a robot.
  ///
  /// \param[in] robot_name
  ///   Name of the robot that the request is targeted at.
  ///
  /// \param[in] location
  ///   Desired relocalization location for the robot.
  ///
  /// \param[in] last_visited_waypoint_index
  ///   The last visited or nearest waypoint index on the navigation graph of
  ///   the robot, for it to continue tracking its progress through the graph.
  ///
  /// \return
  ///   Optional of the command ID for this request. Returns a nullopt if there
  ///   does not exist a robot of the provided name, if the last visited
  ///   waypoint index does not exist in the navigation graph, or if the desired
  ///   relocalization location is too far away from the last visited waypoint.
  std::optional<CommandId> request_relocalization(
    const std::string& robot_name,
    const messages::Location& location,
    std::size_t last_visited_waypoint_index);

  /// Single navigation point within the path.
  struct NavigationPoint
  {
    /// Waypoint index within the navgiation graph.
    std::size_t waypoint_index;

    /// Orientation yaw value in radians for this location. If there is no
    /// preference for the orientation, this field can be left as a nullopt.
    std::optional<double> yaw = std::nullopt;

    /// The time that the robot is expected to wait until on this waypoint
    /// before proceeding. If the robot is expected to move on
    /// immediately, this will be a nullopt.
    std::optional<rmf_traffic::Time> wait_until = std::nullopt;
  };

  /// Sends out a navigation request to a robot.
  ///
  /// \param[in] robot_name
  ///   Name of the robot that the request is targeted at.
  ///
  /// \param[in] path
  ///   Desired path comprised of NavigationPoints that the robot should follow.
  ///
  /// \return
  ///   Optional of the command ID for this request. Returns a nullopt if there
  ///   does not exist a robot of the provided name, if the provided path is
  ///   empty, or if any of the waypoints are non-conforming to the navigation
  ///   graph of the manager.
  std::optional<CommandId> request_navigation(
    const std::string& robot_name,
    const std::vector<NavigationPoint>& path);

  class Implementation;
private:
  Manager();
  rmf_utils::impl_ptr<Implementation> _pimpl;
};

} // namespace free_fleet

#endif // INCLUDE__FREE_FLEET__MANAGER__MANAGER_HPP
