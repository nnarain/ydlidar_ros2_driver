#!/usr/bin/python3
# Copyright 2020, EAIBOT
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

import os


def generate_launch_description():
    share_dir = get_package_share_directory('ydlidar_ros2_driver')
    serial_port = LaunchConfiguration('serial_port')
    frame_id = LaunchConfiguration('frame_id')

    serial_port_declare = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyUSB0',
        description='YDLIDAR serial device path.',
    )
    frame_id_declare = DeclareLaunchArgument(
        'frame_id',
        default_value='laser_frame',
        description='Laser frame id.',
    )

    ydlidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(share_dir, 'launch', 'ydlidar_launch.py')
        ),
        launch_arguments={
            'params_file': os.path.join(share_dir, 'params', 'X2-safe.yaml'),
            'serial_port': serial_port,
            'frame_id': frame_id,
        }.items(),
    )

    return LaunchDescription([
        serial_port_declare,
        frame_id_declare,
        ydlidar_launch,
    ])
