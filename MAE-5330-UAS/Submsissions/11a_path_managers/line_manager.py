"""Provides an implementation of the straight line path manager for waypoint following as described in
   Chapter 11 Algorithm 7
"""

from typing import cast

import numpy as np
from mav_sim.chap11.path_manager_utilities import (
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


def line_manager(state: MsgState, waypoints: MsgWaypoints, ptr_prv: WaypointIndices,
                 path_prv: MsgPath, hs_prv: HalfSpaceParams) \
                -> tuple[MsgPath, HalfSpaceParams, WaypointIndices]:
    """Update for the line manager. Only updates the path and next halfspace under two conditions:
        1) The waypoints are new
        2) In a new halfspace

    Args:
        state: current state of the vehicle
        waypoints: The waypoints to be followed
        ptr_prv: The indices of the waypoints being used for the previous path
        path_prv: The previously commanded path
        hs_prv: The currently active halfspace for switching

    Returns:
        path: The updated path to follow
        hs: The updated halfspace for the next switch
        ptr: The updated index pointer
    """
    # Default the outputs to be the inputs
    path = path_prv
    hs = hs_prv
    ptr = ptr_prv

    # Check if waypoints are new
    if waypoints.flag_waypoints_changed is True:
        waypoints.flag_waypoints_changed = False
        ptr = WaypointIndices()  # Reset the pointers
        (path, hs) = construct_line(waypoints, ptr)
    else:
        # Check if we have entered the next halfspace
        pos = np.array([[state.north], [state.east], [-state.altitude]])
        if inHalfSpace(pos, hs_prv):
            # Increment to the next waypoint
            ptr.increment_pointers(waypoints.num_waypoints)
            # Construct the new line path
            (path, hs) = construct_line(waypoints, ptr)

    # Output the updated path, halfspace, and index pointer
    return (path, hs, ptr)

def construct_line(waypoints: MsgWaypoints, ptr: WaypointIndices) \
    -> tuple[MsgPath, HalfSpaceParams]:
    """Creates a line and switching halfspace. The halfspace assumes that the aggregate
       path will consist of a series of straight lines.

    The line is created from the previous and current waypoints with halfspace defined for
    switching once the current waypoint is reached.

    Args:
        waypoints: The waypoints from which to construct the path
        ptr: The indices of the waypoints being used for the path

    Returns:
        path: The straight-line path to be followed
        hs: The halfspace for switching to the next waypoint
    """
    from mav_sim.chap11.path_manager_utilities import EPSILON

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

    # Construct the path
    path = MsgPath()
    path.plot_updated = False
    path.type = 'line'
    path.airspeed = get_airspeed(waypoints, ptr)
    path.line_origin = previous
    path.line_direction = q_prev

    # Calculate the halfspace normal: n = (q_{i-1} + q_i) / ||q_{i-1} + q_i||
    n = q_prev + q_curr
    n_norm = np.linalg.norm(n)

    # Handle singularity when path folds back (180 degree turn)
    if n_norm < EPSILON:
        # Use the current direction as the normal (switch at the waypoint)
        n = q_prev
    else:
        n = n / n_norm

    # Construct the halfspace
    hs = HalfSpaceParams()
    hs.set(normal=n, point=current)

    return (path, hs)
