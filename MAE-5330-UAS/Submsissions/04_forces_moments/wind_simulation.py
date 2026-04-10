"""
Class to determine wind velocity at any given moment,
calculates a steady wind speed and uses a stochastic
process to represent wind gusts. (Follows section 4.4 in uav book)
"""
from typing import Optional

import numpy as np
from mav_sim.message_types.msg_gust_params import MsgGustParams
from mav_sim.tools import types
from mav_sim.tools.transfer_function import TransferFunction


class WindSimulation:
    """
    Class to determine wind velocity at any given moment,
    calculates a steady wind speed and uses a stochastic
    process to represent wind gusts. (Follows section 4.4 in uav book)
    """
    def __init__(self, Ts: float, gust_params: Optional[MsgGustParams] = None, gust_flag: bool = True) -> None:
        """Initialize steady-state and gust parameters
        """
        # steady state wind defined in the inertial frame
        self._steady_state = np.array([[0., 0., 0.]]).T
        #self._steady_state = np.array([[0., 5., 0.]]).T

        # Initialize gust parameters if not passed in
        if gust_params is None:
            gust_params = MsgGustParams()

        #   Dryden gust model parameters (page 61 of book)
        Va = 25 # must set Va to a constant value
        #
        Lu = gust_params.Lu
        Lv = gust_params.Lv
        Lw = gust_params.Lw
        if gust_flag:
            sigma_u = gust_params.sigma_u
            sigma_v = gust_params.sigma_v
            sigma_w = gust_params.sigma_w
        else:
            sigma_u = 0.0
            sigma_v = 0.0
            sigma_w = 0.0
        
        # Implement H_u(s) transfer function
        # H_u(s) = sigma_u * sqrt(2*Va/(pi*Lu)) * 1/(s + Va/Lu)
        a1 = sigma_u * np.sqrt(2.0 * Va / (np.pi * Lu))
        b1 = Va / Lu
        self.u_w = TransferFunction(num=np.array([[a1]]),
                                     den=np.array([[1, b1]]),
                                     Ts=Ts)
        
        # Implement H_v(s) transfer function
        # H_v(s) = sigma_v * sqrt(3*Va/(pi*Lv)) * (s + Va/(sqrt(3)*Lv)) / (s + Va/Lv)^2
        a2 = sigma_v * np.sqrt(3.0 * Va / (np.pi * Lv))
        b2 = Va / (np.sqrt(3.0) * Lv)
        c2 = Va / Lv
        self.v_w = TransferFunction(num=np.array([[a2, a2 * b2]]),
                                     den=np.array([[1, 2 * c2, c2**2]]),
                                     Ts=Ts)
        
        # Implement H_w(s) transfer function
        # H_w(s) = sigma_w * sqrt(3*Va/(pi*Lw)) * (s + Va/(sqrt(3)*Lw)) / (s + Va/Lw)^2
        a3 = sigma_w * np.sqrt(3.0 * Va / (np.pi * Lw))
        b3 = Va / (np.sqrt(3.0) * Lw)
        c3 = Va / Lw
        self.w_w = TransferFunction(num=np.array([[a3, a3 * b3]]),
                                     den=np.array([[1, 2 * c3, c3**2]]),
                                     Ts=Ts)
        self._Ts = Ts

    def update(self) -> types.WindVector:
        """
        returns a six vector.
           The first three elements are the steady state wind in the inertial frame
           The second three elements are the gust in the body frame
        """
        gust = np.array([[self.u_w.update(np.random.randn())],
                         [self.v_w.update(np.random.randn())],
                         [self.w_w.update(np.random.randn())]])
        return np.concatenate(( self._steady_state, gust ))
