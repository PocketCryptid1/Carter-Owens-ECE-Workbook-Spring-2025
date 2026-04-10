"""path_follower.py implements a class for following a path with a mav
"""
from math import cos, sin

import numpy as np
from mav_sim.message_types.msg_autopilot import MsgAutopilot
from mav_sim.message_types.msg_path import MsgPath
from mav_sim.message_types.msg_state import MsgState
from mav_sim.tools.wrap import wrap


class PathFollower:
    """Class for path following
    """
    def __init__(self) -> None:
        """Initialize path following class
        """
        self.chi_inf = np.radians(50)  # approach angle for large distance from straight-line path
        self.k_path = 0.01 #0.05  # path gain for straight-line path following
        self.k_orbit = 1.# 10.0  # path gain for orbit following
        self.gravity = 9.8
        self.autopilot_commands = MsgAutopilot()  # message sent to autopilot

    def update(self, path: MsgPath, state: MsgState) -> MsgAutopilot:
        """Update the control for following the path

        Args:
            path: path to be followed
            state: current state of the mav

        Returns:
            autopilot_commands: Commands to autopilot for following the path
        """
        if path.type == 'line':
            self.autopilot_commands = follow_straight_line(path=path, state=state, k_path=self.k_path, chi_inf=self.chi_inf)
        elif path.type == 'orbit':
            self.autopilot_commands = follow_orbit(path=path, state=state, k_orbit=self.k_orbit, gravity=self.gravity)
        return self.autopilot_commands

def follow_straight_line(path: MsgPath, state: MsgState, k_path: float, chi_inf: float) -> MsgAutopilot:
    """Calculate the autopilot commands for following a straight line

    Args:
        path: straight-line path to be followed
        state: current state of the mav
        k_path: convergence gain for converging to the path
        chi_inf: Angle to take towards path when at an infinite distance from the path

    Returns:
        autopilot_commands: the commands required for executing the desired line
    """
    # Initialize the output
    autopilot_commands = MsgAutopilot()

    # Extract path parameters
    q = path.line_direction  # unit direction vector (3x1)
    r = path.line_origin     # origin point (3x1)
    qn = float(q.item(0))
    qe = float(q.item(1))
    qd = float(q.item(2))
    rn = float(r.item(0))
    re = float(r.item(1))
    rd = float(r.item(2))

    # Aircraft position in NED
    pn = state.north
    pe = state.east

    # Course angle of the path line - wrap to be within pi of state.chi
    chi_q = float(np.arctan2(qe, qn))
    chi_q = wrap(chi_q, state.chi)

    # Path error in the path frame (rotate about down axis by chi_q)
    e_py = -sin(chi_q) * (pn - rn) + cos(chi_q) * (pe - re)  # cross-track error

    # Course command
    chi_d = chi_q - chi_inf * (2.0 / np.pi) * float(np.arctan(k_path * e_py))

    # Altitude command using projection onto path plane
    # n = (q x k) / ||q x k|| where k = (0,0,1)^T => n = (qe, -qn, 0) / ||q_ne||
    qne = np.sqrt(qn**2 + qe**2)
    nn = qe / qne
    ne = -qn / qne
    # Error vector ep = p - r (NED)
    ep_n = pn - rn
    ep_e = pe - re
    # ep dot n (n has zero down component)
    ep_dot_n = ep_n * nn + ep_e * ne
    # s = ep - (ep.n)*n => s_ne components
    s_n = ep_n - ep_dot_n * nn
    s_e = ep_e - ep_dot_n * ne
    s_ne_norm = np.sqrt(s_n**2 + s_e**2)
    # h_c = -r_d + ||s_ne|| * (-q_d / ||q_ne||)
    h_d = -rd + s_ne_norm * (-qd / qne)

    # Populate autopilot commands
    autopilot_commands.airspeed_command = float(path.airspeed)
    autopilot_commands.course_command = float(chi_d)
    autopilot_commands.altitude_command = float(h_d)
    autopilot_commands.phi_feedforward = 0.0

    return autopilot_commands


def follow_orbit(path: MsgPath, state: MsgState, k_orbit: float, gravity: float) -> MsgAutopilot:
    """Calculate the autopilot commands for following a circular path

    Args:
        path: circular orbit to be followed
        state: current state of the mav
        k_orbit: Convergence gain for reducing error to orbit
        gravity: Gravity constant

    Returns:
        autopilot_commands: the commands required for executing the desired orbit
    """

    # Initialize the output
    autopilot_commands = MsgAutopilot()

    # Orbit parameters
    c = path.orbit_center   # center (3x1)
    rho = path.orbit_radius
    cn = float(c.item(0))
    ce = float(c.item(1))
    cd = float(c.item(2))

    # Aircraft position
    pn = state.north
    pe = state.east

    # Orbit direction: +1 for CW, -1 for CCW
    lam = 1.0 if path.orbit_direction == 'CW' else -1.0

    # Distance from aircraft to orbit center (in NE plane)
    d = float(np.sqrt((pn - cn)**2 + (pe - ce)**2))

    # Angular position of aircraft relative to orbit center (wrapped near current course)
    varphi = float(np.arctan2(pe - ce, pn - cn))
    varphi = wrap(varphi, state.chi)

    # Course command
    chi_o = varphi + lam * (np.pi / 2.0 + float(np.arctan(k_orbit * (d - rho) / rho)))

    # Altitude command (convert down coordinate to altitude)
    h_c = float(-cd)

    # Feedforward roll: only apply when (d - rho) / rho < 10
    if (d - rho) / rho < 10:
        phi_ff = float(lam * np.arctan(path.airspeed**2 / (gravity * rho)))
    else:
        phi_ff = 0.0

    # Populate autopilot commands
    autopilot_commands.airspeed_command = float(path.airspeed)
    autopilot_commands.course_command = float(chi_o)
    autopilot_commands.altitude_command = h_c
    autopilot_commands.phi_feedforward = phi_ff

    return autopilot_commands
