"""Provides an implementation of the fillet path manager for waypoint following as described in
   Chapter 11 Algorithm 8
"""
from typing import cast

import numpy as np
from mav_sim.chap11.path_manager_utilities import (
    EPSILON,
    HalfSpaceParams,
    WaypointIndices,
    extract_waypoints,
    get_airspeed,
    inHalfSpace,
)
from mav_sim.message_types.msg_path import MsgPath
from mav_sim.message_types.msg_state import MsgState
from mav_sim.message_types.msg_waypoints import MsgWaypoints
from mav_sim.tools.types import NP_MAT


def fillet_manager(state: MsgState, waypoints: MsgWaypoints, ptr_prv: WaypointIndices,
                 path_prv: MsgPath, hs_prv: HalfSpaceParams, radius: float, manager_state: int) \
                -> tuple[MsgPath, HalfSpaceParams, WaypointIndices, int]:

    """Update for the fillet manager.
       Updates state machine if the MAV enters into the next halfspace.

    Args:
        state: current state of the vehicle
        waypoints: The waypoints to be followed
        ptr_prv: The indices that were being used on the previous iteration (i.e., current waypoint
                 inidices being followed when manager called)
        hs_prv: The previous halfspace being looked for (i.e., the current halfspace when manager called)
        radius: minimum radius circle for the mav
        manager_state: Integer state of the manager
                Value of 1 corresponds to following the straight line path
                Value of 2 corresponds to following the arc between straight lines

    Returns:
        path (MsgPath): Path to be followed
        hs (HalfSpaceParams): Half space parameters corresponding to the next change in state
        ptr (WaypointIndices): Indices of the current waypoint being followed
        manager_state (int): The current state of the manager

    """
    # Default the outputs to be the inputs
    path = path_prv
    hs = hs_prv
    ptr = ptr_prv

    # Check if waypoints are new
    if waypoints.flag_waypoints_changed is True:
        waypoints.flag_waypoints_changed = False
        ptr = WaypointIndices()  # Reset the pointers
        manager_state = 1  # Initialize to line following state
        (path, hs) = construct_fillet_line(waypoints, ptr, radius)
    else:
        # Get current position
        pos = np.array([[state.north], [state.east], [-state.altitude]])

        # Check if we have entered the next halfspace
        if inHalfSpace(pos, hs_prv):
            if manager_state == 1:
                # Transition from line to circle
                manager_state = 2
                (path, hs) = construct_fillet_circle(waypoints, ptr, radius)
            else:  # manager_state == 2
                # Transition from circle to line, increment waypoint
                manager_state = 1
                ptr.increment_pointers(waypoints.num_waypoints)
                (path, hs) = construct_fillet_line(waypoints, ptr, radius)

    return (path, hs, ptr, manager_state)

def construct_fillet_line(waypoints: MsgWaypoints, ptr: WaypointIndices, radius: float) \
    -> tuple[MsgPath, HalfSpaceParams]:
    """Define the line on a fillet and a halfspace for switching to the next fillet curve.

    The line is created from the previous and current waypoints with halfspace defined for
    switching once a circle of the specified radius can be used to transition to the next line segment.

    Args:
        waypoints: The waypoints to be followed
        ptr: The indices of the waypoints being used for the path
        radius: minimum radius circle for the mav

    Returns:
        path: The straight-line path to be followed
        hs: The halfspace for switching to the next waypoint
    """
    # Extract the waypoints (w_{i-1}, w_i, w_{i+1})
    (previous, current, next_wp) = extract_waypoints(waypoints=waypoints, ptr=ptr)

    # Calculate unit vectors for path directions
    # q_{i-1}: direction from w_{i-1} to w_i
    q_prev = current - previous
    q_prev_norm = np.linalg.norm(q_prev)
    if q_prev_norm > EPSILON:
        q_prev = q_prev / q_prev_norm
    else:
        q_prev = np.array([[1.0], [0.0], [0.0]])  # Default direction

    # q_i: direction from w_i to w_{i+1}
    q_curr = next_wp - current
    q_curr_norm = np.linalg.norm(q_curr)
    if q_curr_norm > EPSILON:
        q_curr = q_curr / q_curr_norm
    else:
        q_curr = q_prev  # Continue in same direction

    # Calculate the turn angle varrho (rho)
    # cos(rho) = -q_{i-1} dot q_i (angle between the two path segments)
    cos_varrho = -float(np.dot(q_prev.T, q_curr).item())
    # Clamp to valid range for acos
    cos_varrho = np.clip(cos_varrho, -1.0, 1.0)
    varrho = np.arccos(cos_varrho)

    # Construct the path
    path = MsgPath()
    path.plot_updated = False
    path.type = 'line'
    path.airspeed = get_airspeed(waypoints, ptr)
    path.line_origin = previous
    path.line_direction = q_prev

    # Calculate halfspace point z_i = w_i - (R/tan(varrho/2)) * q_{i-1}
    # Handle singularity when varrho is close to 0 (straight line) or pi (fold back)
    if abs(np.sin(varrho / 2)) < EPSILON:
        # Straight line case: switch at the waypoint
        z = current
    elif abs(np.tan(varrho / 2)) < EPSILON:
        # Path folds back: switch at the waypoint
        z = current
    else:
        d = radius / np.tan(varrho / 2)
        z = current - d * q_prev

    # Halfspace normal is q_{i-1} (switch when passing z along the line direction)
    hs = HalfSpaceParams()
    hs.set(normal=q_prev, point=z)

    return (path, hs)

def construct_fillet_circle(waypoints: MsgWaypoints, ptr: WaypointIndices, radius: float) \
    -> tuple[MsgPath, HalfSpaceParams]:
    """Define the circle on a fillet

    Args:
        waypoints: The waypoints to be followed
        ptr: The indices of the waypoints being used for the path
        radius: minimum radius circle for the mav

    Returns:
        path: The straight-line path to be followed
        hs: The halfspace for switching to the next waypoint
    """
    # Extract the waypoints (w_{i-1}, w_i, w_{i+1})
    (previous, current, next_wp) = extract_waypoints(waypoints=waypoints, ptr=ptr)

    # Calculate unit vectors for path directions
    # q_{i-1}: direction from w_{i-1} to w_i
    q_prev = current - previous
    q_prev_norm = np.linalg.norm(q_prev)
    if q_prev_norm > EPSILON:
        q_prev = q_prev / q_prev_norm
    else:
        q_prev = np.array([[1.0], [0.0], [0.0]])  # Default direction

    # q_i: direction from w_i to w_{i+1}
    q_curr = next_wp - current
    q_curr_norm = np.linalg.norm(q_curr)
    if q_curr_norm > EPSILON:
        q_curr = q_curr / q_curr_norm
    else:
        q_curr = q_prev  # Continue in same direction

    # Calculate the turn angle varrho (rho)
    # cos(rho) = -q_{i-1} dot q_i
    cos_varrho = -float(np.dot(q_prev.T, q_curr).item())
    cos_varrho = np.clip(cos_varrho, -1.0, 1.0)
    varrho = np.arccos(cos_varrho)

    # Construct the path
    path = MsgPath()
    path.plot_updated = False
    path.type = 'orbit'
    path.airspeed = get_airspeed(waypoints, ptr)
    path.orbit_radius = radius

    # Handle singularity when varrho is close to 0 (straight line) or pi (fold back)
    if abs(np.sin(varrho / 2)) < EPSILON:
        # Straight line case: no turn needed, create a minimal circle at current waypoint
        path.orbit_center = current
        path.orbit_direction = 'CW'
        z = current
        n = q_curr
    else:
        # Calculate the fillet center
        # Direction to center: (q_{i-1} - q_i) / ||q_{i-1} - q_i||
        q_diff = q_prev - q_curr
        q_diff_norm = np.linalg.norm(q_diff)

        if q_diff_norm < EPSILON:
            # This happens when q_prev ≈ q_curr (continuing in same direction)
            # This is the terminal waypoint case where next is extrapolated
            # Use a perpendicular direction: reflect q_prev across the E axis
            # This gives [-N, E, -D] from [N, E, D]
            q_perp = np.array([[-q_prev[0, 0]], [q_prev[1, 0]], [-q_prev[2, 0]]])
            center = current + radius * q_perp
            path.orbit_center = center
            path.orbit_direction = 'CW'
            z = current
            n = q_curr
        else:
            q_diff = q_diff / q_diff_norm
            # Center is at distance R/sin(varrho/2) from w_i
            c_dist = radius / np.sin(varrho / 2)
            center = current - c_dist * q_diff
            path.orbit_center = center

            # Determine orbit direction
            # The orbit direction depends on whether we turn left (CCW) or right (CW)
            # when viewed looking DOWN (positive D in NED frame)
            # 
            # Cross product q_prev × q_curr gives the rotation axis
            # If the z-component is non-negative, the rotation is CW when viewed from above
            # If the z-component is negative, the rotation is CCW when viewed from above
            turn_axis = np.cross(q_prev.flatten(), q_curr.flatten())
            
            # Non-negative z component (including zero) means CW, negative means CCW
            if turn_axis[2] >= 0:
                path.orbit_direction = 'CW'
            else:
                path.orbit_direction = 'CCW'

            # Halfspace point z = w_i + (R/tan(varrho/2)) * q_i
            d = radius / np.tan(varrho / 2)
            z = current + d * q_curr
            n = q_curr

    # Define the switching halfspace
    hs = HalfSpaceParams()
    hs.set(normal=n, point=z)

    return (path, hs)
