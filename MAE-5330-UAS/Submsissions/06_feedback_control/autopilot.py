"""
autopilot block for mavsim_python
    - Beard & McLain, PUP, 2012
    - Last Update:
        2/6/2019 - RWB
        12/21 - GND
"""
import mav_sim.parameters.control_parameters as AP
import numpy as np
from mav_sim.chap6.pd_control_with_rate import PDControlWithRate
from mav_sim.chap6.pi_control import PIControl
from mav_sim.chap6.tf_control import TFControl
from mav_sim.message_types.msg_autopilot import MsgAutopilot
from mav_sim.message_types.msg_delta import MsgDelta
from mav_sim.message_types.msg_state import MsgState

# from mav_sim.tools.transfer_function import TransferFunction
from mav_sim.tools.wrap import saturate, wrap


class Autopilot:
    """Creates an autopilot for controlling the mav to desired values
    """
    def __init__(self, ts_control: float) -> None:
        """Initialize the lateral and longitudinal controllers

        Args:
            ts_control: time step for the control
        """

        # instantiate lateral-directional controllers
        # Roll from aileron: PD control with rate feedback
        self.roll_from_aileron = PDControlWithRate(
            kp=AP.roll_kp,
            kd=AP.roll_kd,
            limit=np.radians(45.0)
        )
        # Course from roll: PI control
        self.course_from_roll = PIControl(
            kp=AP.course_kp,
            ki=AP.course_ki,
            Ts=ts_control,
            limit=np.radians(30.0)
        )
        # Yaw damper: Transfer function control
        self.yaw_damper = TFControl(
            k=AP.yaw_damper_kr,
            n0=0.0,
            n1=1.0,
            d0=AP.yaw_damper_p_wo,
            d1=1.0,
            Ts=ts_control,
            limit=1.0
        )

        # instantiate longitudinal controllers
        # Pitch from elevator: PD control with rate feedback
        self.pitch_from_elevator = PDControlWithRate(
            kp=AP.pitch_kp,
            kd=AP.pitch_kd,
            limit=np.radians(45.0)
        )
        # Altitude from pitch: PI control
        self.altitude_from_pitch = PIControl(
            kp=AP.altitude_kp,
            ki=AP.altitude_ki,
            Ts=ts_control,
            limit=np.radians(30.0)
        )
        # Airspeed from throttle: PI control
        self.airspeed_from_throttle = PIControl(
            kp=AP.airspeed_throttle_kp,
            ki=AP.airspeed_throttle_ki,
            Ts=ts_control,
            limit=1.0
        )
        self.commanded_state = MsgState()

    def update(self, cmd: MsgAutopilot, state: MsgState) -> tuple[MsgDelta, MsgState]:
        """Given a state and autopilot command, compute the control to the mav

        Args:
            cmd: command to the autopilot
            state: current state of the mav

        Returns:
            delta: low-level flap commands
            commanded_state: the state being commanded
        """

        # ---- Lateral autopilot ----
        # Course hold: compute commanded roll angle phi_c from course error
        # Wrap the course command to be within +-pi of current course
        chi_c = wrap(cmd.course_command, state.chi)
        # PI control: course error -> phi_c, with feedforward term
        phi_c = cmd.phi_feedforward + self.course_from_roll.update(y_ref=chi_c, y=state.chi)
        # Saturate phi_c to +/- 30 degrees
        phi_c = saturate(phi_c, -np.radians(30.0), np.radians(30.0))

        # Roll hold: compute aileron command from roll error
        delta_a = self.roll_from_aileron.update(y_ref=phi_c, y=state.phi, ydot=state.p)

        # Yaw damper: compute rudder command from yaw rate
        delta_r = self.yaw_damper.update(y=state.r)

        # ---- Longitudinal autopilot ----
        # Saturate altitude command to be within altitude_zone of current altitude
        h_c = saturate(cmd.altitude_command,
                       state.altitude - AP.altitude_zone,
                       state.altitude + AP.altitude_zone)

        # Altitude hold: compute commanded pitch angle theta_c from altitude error
        theta_c = self.altitude_from_pitch.update(y_ref=h_c, y=state.altitude)

        # Pitch hold: compute elevator command from pitch error
        delta_e = self.pitch_from_elevator.update(y_ref=theta_c, y=state.theta, ydot=state.q)

        # Airspeed hold using throttle: compute throttle command from airspeed error
        # Assume nominal airspeed contribution is zero (integrator will learn it)
        delta_t = self.airspeed_from_throttle.update(y_ref=cmd.airspeed_command, y=state.Va)
        # Do not allow negative thrust
        delta_t = saturate(delta_t, 0.0, 1.0)

        # construct control outputs and commanded states
        delta = MsgDelta(elevator=delta_e,
                         aileron=delta_a,
                         rudder=delta_r,
                         throttle=delta_t)
        self.commanded_state.altitude = cmd.altitude_command
        self.commanded_state.Va = cmd.airspeed_command
        self.commanded_state.phi = phi_c
        self.commanded_state.theta = theta_c
        self.commanded_state.chi = cmd.course_command
        return delta, self.commanded_state.copy()
