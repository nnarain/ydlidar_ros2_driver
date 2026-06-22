#!/usr/bin/python3
# Copyright 2020, EAIBOT
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

import os


def generate_launch_description():
    share_dir = get_package_share_directory('ydlidar_ros2_driver')
    parameter_file = LaunchConfiguration('params_file')
    serial_port = LaunchConfiguration('serial_port')
    frame_id = LaunchConfiguration('frame_id')

    params_declare = DeclareLaunchArgument('params_file',
                                           default_value=os.path.join(
                                               share_dir, 'params', 'ydlidar.yaml'),
                                           description='Path to the ROS2 parameters file to use.')

    serial_port_declare = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ydlidar',
        description='YDLIDAR serial device path (overrides params file `port`).',
    )
    frame_id_declare = DeclareLaunchArgument(
        'frame_id',
        default_value='laser_frame',
        description='Laser frame id (overrides params file `frame_id`).',
    )

    # Updated for ROS2 Jazzy - use 'executable' instead of 'node_executable'
    # and 'name' instead of 'node_name', 'namespace' instead of 'node_namespace'
    driver_node = LifecycleNode(package='ydlidar_ros2_driver',
                                executable='ydlidar_ros2_driver_node',
                                name='ydlidar_ros2_driver_node',
                                output='screen',
                                emulate_tty=True,
                                parameters=[parameter_file, {
                                    # Later entries in the list override values from the YAML file.
                                    'port': serial_port,
                                    'frame_id': frame_id,
                                }],
                                namespace='/',
                                )
    
    tf2_node = Node(package='tf2_ros',
                    executable='static_transform_publisher',
                    name='static_tf_pub_laser',
                    arguments=['--x', '0', '--y', '0', '--z', '0.02',
                               '--roll', '0', '--pitch', '0', '--yaw', '0',
                               '--frame-id', 'base_link', '--child-frame-id', frame_id],
                    )

    return LaunchDescription([
        params_declare,
        serial_port_declare,
        frame_id_declare,
        driver_node,
        tf2_node,
    ])
