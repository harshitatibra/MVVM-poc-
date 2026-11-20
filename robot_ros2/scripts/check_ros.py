#!/usr/bin/env python3
"""Run while the simulator is active. Exercises real ROS messages/services."""
import math
import time

import rclpy
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
from qt_robot_interfaces.msg import Telemetry
from qt_robot_interfaces.srv import RobotCommand


def main():
    rclpy.init()
    node = rclpy.create_node("poc_integration_check")
    samples = []
    qos = QoSProfile(depth=1, reliability=ReliabilityPolicy.RELIABLE,
                     durability=DurabilityPolicy.VOLATILE)
    subscription = node.create_subscription(Telemetry, "/quadruped/telemetry", samples.append, qos)
    client = node.create_client(RobotCommand, "/quadruped/command")

    def wait_for(predicate, description, timeout=10.0):
        deadline = time.monotonic() + timeout
        while not predicate():
            if time.monotonic() >= deadline:
                raise RuntimeError("Timed out: " + description)
            rclpy.spin_once(node, timeout_sec=0.1)

    def command(name, expected=True):
        request = RobotCommand.Request()
        request.command = name
        future = client.call_async(request)
        wait_for(future.done, "command response: " + name)
        reply = future.result()
        if reply.accepted != expected:
            raise RuntimeError(f"Unexpected response to {name}: {reply.message}")
        print(f"PASS: {name}: {reply.message}")
        return reply

    def next_matching(predicate, description):
        # Discard previous samples; wait for a post-command telemetry update.
        samples.clear()
        wait_for(lambda: bool(samples) and predicate(samples[-1]), description)

    try:
        wait_for(lambda: len(samples) >= 3, "three telemetry messages")
        if not samples[-1].sequence > samples[0].sequence:
            raise RuntimeError("Sequence did not advance")
        for sample in samples:
            if not (0 <= sample.battery_percent <= 100 and
                    math.isfinite(sample.cpu_temperature_c) and
                    0 <= sample.dummy_value <= 100):
                raise RuntimeError("Invalid telemetry values")
        print("PASS: telemetry received with advancing sequence and valid values")
        wait_for(client.service_is_ready, "command service discovery")
        command("ping")
        command("stand")
        next_matching(lambda msg: msg.mode == "STANDING", "standing telemetry")
        command("sit")
        next_matching(lambda msg: msg.mode == "SITTING", "sitting telemetry")
        command("low_battery")
        next_matching(lambda msg: msg.health == "LOW BATTERY" and msg.battery_percent < 20,
                      "low battery telemetry")
        command("not_a_command", expected=False)
        command("reset")
        next_matching(lambda msg: msg.health == "OK" and msg.battery_percent > 99,
                      "reset telemetry")
        print("PASS: ROS telemetry, command acceptance/rejection, and state feedback")
    finally:
        node.destroy_subscription(subscription)
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
