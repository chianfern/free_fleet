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

#include <chrono>
#include <memory>

#include <rmf_utils/catch.hpp>

#include <free_fleet/manager/Manager.hpp>
#include <free_fleet/messages/RobotMode.hpp>
#include <free_fleet/messages/Waypoint.hpp>
#include <free_fleet/messages/Location.hpp>
#include <free_fleet/manager/SimpleCoordinateTransformer.hpp>

#include "src/manager/internal_Manager.hpp"
#include "src/manager/internal_RobotInfo.hpp"

#include "mock_ServerMiddleware.hpp"

SCENARIO("Test Manager API")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new free_fleet::MockServerMiddleware());
  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    []() { return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo&) {};

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  GIVEN("Starting with initial conditions, running 5 times")
  {
    for (int i = 0; i < 5; ++i)
      CHECK_NOTHROW(manager->run_once());
  }

  GIVEN("Started with no robots")
  {
    for (int i = 0; i < 5; ++i)
      CHECK_NOTHROW(manager->run_once());

    auto robot_names = manager->robot_names();
    CHECK(robot_names.empty());

    auto info = manager->robot("random");
    CHECK(!info);

    auto all_info = manager->all_robots();
    CHECK(all_info.empty());
  }

  GIVEN("Sending requests with no robots")
  {
    auto robot_names = manager->robot_names();
    CHECK(robot_names.empty());

    std::string rn = "test_robot";

    auto id = manager->request_pause(rn);
    CHECK(!id.has_value());

    id = manager->request_resume(rn);
    CHECK(!id.has_value());

    id = manager->request_dock(rn, "dock_name");
    CHECK(!id.has_value());

    id = manager->request_relocalization(
      rn,
      free_fleet::messages::Location("test_map", {0.0, 0.0}, 0.0),
      0);
    CHECK(!id.has_value());

    using NavigationPoint = free_fleet::Manager::NavigationPoint;
    id = manager->request_navigation(
      rn,
      {
        NavigationPoint{0, 0.0},
        NavigationPoint{1, 0.0}
      });
    CHECK(!id.has_value());
  }
}

SCENARIO("Testing manager API with dummy robots")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);
  graph->add_waypoint(test_map_name, {100, 100});

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new free_fleet::MockServerMiddleware());
  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    []() { return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo&) {};

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  using RobotMode = free_fleet::messages::RobotMode;
  using Location = free_fleet::messages::Location;
  using RobotState = free_fleet::messages::RobotState;
  rmf_traffic::Time initial_time = std::chrono::steady_clock::now();

  auto& impl = free_fleet::Manager::Implementation::get(*manager);

  RobotState state_1(
    std::chrono::steady_clock::now(),
    "test_robot_1",
    "test_model",
    0,
    false,
    RobotMode(RobotMode::Mode::Idle),
    1.0,
    Location(test_map_name, {0.0, 0.0}, 0.0),
    0);
  auto robot_info_1 = free_fleet::manager::RobotInfo::Implementation::make(
    state_1,
    graph,
    initial_time);
  REQUIRE(robot_info_1);

  RobotState state_2(
    std::chrono::steady_clock::now(),
    "test_robot_2",
    "test_model",
    0,
    false,
    RobotMode(RobotMode::Mode::Idle),
    1.0,
    Location(test_map_name, {0.0, 0.0}, 0.0),
    0);
  auto robot_info_2 = free_fleet::manager::RobotInfo::Implementation::make(
    state_2,
    graph,
    initial_time);
  REQUIRE(robot_info_2);

  RobotState state_3(
    std::chrono::steady_clock::now(),
    "test_robot_3",
    "test_model",
    0,
    false,
    RobotMode(RobotMode::Mode::Idle),
    1.0,
    Location(test_map_name, {0.0, 0.0}, 0.0),
    0);
  auto robot_info_3 = free_fleet::manager::RobotInfo::Implementation::make(
    state_3,
    graph,
    initial_time);
  REQUIRE(robot_info_3);

  impl.robots[robot_info_1->name()] = robot_info_1;
  impl.robots[robot_info_2->name()] = robot_info_2;
  impl.robots[robot_info_3->name()] = robot_info_3;

  GIVEN("Basic API with dummy robots")
  {
    auto names = manager->robot_names();
    CHECK(names.size() == 3);

    auto all_info = manager->all_robots();
    CHECK(all_info.size() == 3);

    auto info = manager->robot("test_robot_1");
    REQUIRE(info);
    CHECK(info->name() == "test_robot_1");

    info = manager->robot("test_robot_2");
    REQUIRE(info);
    CHECK(info->name() == "test_robot_2");
  }

  GIVEN("Sending pause request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_pause("test_robot_1");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_pause("test_robot_10");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_pause("test_robot_2");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_pause("test_robot_3");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending resume request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_resume("test_robot_1");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_resume("test_robot_10");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_resume("test_robot_2");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_resume("test_robot_3");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending dock request to dummy robots")
  {
    // Valid mode
    auto id_1 = manager->request_dock("test_robot_1", "mock_dock");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid robot
    auto id_2 = manager->request_dock("test_robot_10", "mock_dock");
    CHECK(!id_2.has_value());

    // Subsequent mode requests
    auto id_3 = manager->request_dock("test_robot_2", "mock_dock");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 2);

    auto id_4 = manager->request_dock("test_robot_3", "mock_dock");
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 3);
  }

  GIVEN("Sending relocalization requests to dummy robots")
  {
    // Valid relocalization
    free_fleet::messages::Location valid_location(
      test_map_name, {0.0, 0.0}, 0.0);
    auto id_1 = manager->request_relocalization(
      "test_robot_1",
      valid_location,
      0);
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid waypoint index
    auto id_2 = manager->request_relocalization(
      "test_robot_2",
      valid_location,
      100);
    CHECK(!id_2.has_value());

    // Last visited waypoint too far away
    auto id_3 = manager->request_relocalization(
      "test_robot_3",
      valid_location,
      5);
    CHECK(!id_3.has_value());

    // Subsequent requests
    auto id_4 = manager->request_relocalization(
      "test_robot_2",
      valid_location,
      0);
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 2);

    auto id_5 = manager->request_relocalization(
      "test_robot_3",
      valid_location,
      0);
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 3);
  }

  GIVEN("Sending navigation request to dummy robots")
  {
    using NavigationPoint = free_fleet::Manager::NavigationPoint;

    // Valid waypoints
    auto id_1 = manager->request_navigation(
      "test_robot_2",
      {NavigationPoint{0, 0.0}, NavigationPoint{1, 0.0}});
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Invalid waypoints
    free_fleet::messages::Waypoint invalid_waypoint(
      100, Location("invalid_map_name", {0.0, 0.0}, 0.0));
    auto id_2 = manager->request_navigation(
      "test_robot_3",
      {NavigationPoint{0, 0.0}, NavigationPoint{100, 0.0}});
    CHECK(!id_2.has_value());

    // Empty path
    auto id_3 = manager->request_navigation(
      "test_robot_3",
      {});
    CHECK(!id_3.has_value());

    // Invalid robot
    auto id_4 = manager->request_navigation(
      "test_robot_30",
      {NavigationPoint{0, 0.0}, NavigationPoint{0, 0.0}});
    CHECK(!id_4.has_value());

    // Subsequent navigation request
    auto id_5 = manager->request_navigation(
      "test_robot_1",
      {NavigationPoint{0, 0.0}, NavigationPoint{1, 0.0}});
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 2);

    auto id_6 = manager->request_navigation(
      "test_robot_3",
      {NavigationPoint{0, 0.0}, NavigationPoint{1, 0.0}});
    REQUIRE(id_6.has_value());
    CHECK(id_6.value() == 3);
  }

  GIVEN("Sending subsequent requests of different types")
  {
    using NavigationPoint = free_fleet::Manager::NavigationPoint;

    // Dock
    auto id_1 = manager->request_dock("test_robot_1", "mock_dock");
    REQUIRE(id_1.has_value());
    CHECK(id_1.value() == 1);

    // Pause
    auto id_2 = manager->request_pause("test_robot_1");
    REQUIRE(id_2.has_value());
    CHECK(id_2.value() == 2);

    // Resume
    auto id_3 = manager->request_resume("test_robot_1");
    REQUIRE(id_3.has_value());
    CHECK(id_3.value() == 3);

    // Navigation
    auto id_4 = manager->request_navigation(
      "test_robot_2",
      {NavigationPoint{0, 0.0}, NavigationPoint{1, 0.0}});
    REQUIRE(id_4.has_value());
    CHECK(id_4.value() == 4);

    // Relocalization
    auto id_5 = manager->request_relocalization(
      "test_robot_3",
      Location(test_map_name, {0.0, 10.0}, 0.0),
      3);
    REQUIRE(id_5.has_value());
    CHECK(id_5.value() == 5);

    // Valid Relocalization
    auto id_6 = manager->request_relocalization(
      "test_robot_1",
      Location(test_map_name, {0.0, 0.0}, 0.0),
      0);
    REQUIRE(id_6.has_value());
    CHECK(id_6.value() == 6);
  }
}

SCENARIO("Testing update robot callback with dummy robot")
{
  const std::string fleet_name = "test_fleet";

  const std::string test_map_name = "test_level";
  std::shared_ptr<rmf_traffic::agv::Graph> graph(new rmf_traffic::agv::Graph);
  graph->add_waypoint(test_map_name, {0, 0});
  graph->add_waypoint(test_map_name, {10, 0});
  graph->add_waypoint(test_map_name, {-10, 0});
  graph->add_waypoint(test_map_name, {0, 10});
  graph->add_waypoint(test_map_name, {0, -10});
  graph->add_lane(0, 1);
  graph->add_lane(1, 0);
  graph->add_lane(0, 2);
  graph->add_lane(2, 0);
  graph->add_lane(0, 3);
  graph->add_lane(3, 0);
  graph->add_lane(0, 4);
  graph->add_lane(4, 0);
  graph->add_waypoint(test_map_name, {100, 100});

  std::unique_ptr<free_fleet::transport::ServerMiddleware> m(
    new free_fleet::MockServerMiddleware());

  auto ct = free_fleet::manager::SimpleCoordinateTransformer::make(
    1.0,
    0.0,
    0.0,
    0.0);
  free_fleet::Manager::TimeNow time_now_fn =
    []() { return std::chrono::steady_clock::now(); };
  free_fleet::Manager::RobotUpdatedCallback cb =
    [](const free_fleet::manager::RobotInfo& updated_robot_info)
    {
      CHECK(updated_robot_info.name() == "test_robot");
    };

  auto manager = free_fleet::Manager::make(
    fleet_name,
    graph,
    std::move(m),
    ct,
    time_now_fn,
    cb);
  REQUIRE(manager);

  free_fleet::messages::RobotState initial_state(
    std::chrono::steady_clock::now(),
    "test_robot",
    "test_model",
    1,
    false,
    free_fleet::messages::RobotMode(
      free_fleet::messages::RobotMode::Mode::Idle),
    1.0,
    free_fleet::messages::Location("test_map", {0.0, 0.0}, 0.0),
    0);
  rmf_traffic::Time initial_time = std::chrono::steady_clock::now();

  auto& impl = free_fleet::Manager::Implementation::get(*manager);

  auto robot_info = free_fleet::manager::RobotInfo::Implementation::make(
    initial_state,
    graph,
    initial_time);
  REQUIRE(robot_info);

  impl.robots[robot_info->name()] = robot_info;
  manager->run_once();
}
