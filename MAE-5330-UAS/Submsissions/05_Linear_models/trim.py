"""
compute_trim
    - Chapter 5 assignment for Beard & McLain, PUP, 2012
    - Update history:
        12/29/2018 - RWB
        1/2022 - GND
"""

import numpy as np
from mav_sim.chap3.mav_dynamics_euler import (
    IND_EULER,
    derivatives_euler,
    euler_state_to_quat_state,
    quat_state_to_euler_state,
)
from mav_sim.chap4.mav_dynamics import forces_moments, update_velocity_data
from mav_sim.message_types.msg_delta import MsgDelta
from mav_sim.tools import types
from scipy.optimize import Bounds, minimize


def compute_trim(state0: types.DynamicState, Va: float, gamma: float, R: float = np.inf) -> tuple[types.DynamicState, MsgDelta]:
    """Compute the trim equilibrium given the airspeed and flight path angle

    Args:
        state0: An initial guess at the state
        Va: air speed
        gamma: flight path angle
        R: radius - np.inf corresponds to a straight line

    Returns:
        trim_state: The resulting trim trajectory state
        trim_input: The resulting trim trajectory inputs
    """
    # Convert the state to euler representation
    state0_euler = quat_state_to_euler_state(state0)

    # Calculate the trim
    trim_state_euler, trim_input = compute_trim_euler(state0=state0_euler, Va=Va, gamma=gamma, R=R)

    # Convert and output the returned value
    trim_state = euler_state_to_quat_state(trim_state_euler)
    return trim_state, trim_input

def compute_trim_euler(state0: types.DynamicStateEuler, Va: float, gamma: float, R: float) \
    -> tuple[types.DynamicStateEuler, MsgDelta]:
    """Compute the trim equilibrium given the airspeed, flight path angle, and radius

    Args:
        state0: An initial guess at the state
        Va: air speed
        gamma: flight path angle
        R: radius - np.inf corresponds to a straight line

    Returns:
        trim_state: The resulting trim trajectory state
        trim_input: The resulting trim trajectory inputs
    """
    # define initial state and input
    delta0 = MsgDelta(elevator=0., aileron=0., rudder=0., throttle=0.5)
    x0 = np.concatenate((state0, delta0.to_array()), axis=0)

    # define equality constraints
    cons = ({'type': 'eq',
             'fun': lambda x: np.array([
                                # magnitude of velocity vector is Va
                                velocity_constraint(x=x, Va_desired=Va),
                                ]),
             'jac': lambda x: np.array([
                                velocity_constraint_partial(x=x)
                                ])
             })
    # Define the bounds
    eps = 1e-12 # Small number to force equality constraint to be feasible during optimization (bug in scipy)
    lb, ub = variable_bounds(state0=state0, eps=eps)

    # solve the minimization problem to find the trim states and inputs
    psi_weight = 100000. # Weight on convergence of psi
    res = minimize(trim_objective_fun, x0[:,0], method='SLSQP', args=(Va, gamma, R, psi_weight), bounds=Bounds(lb=lb, ub=ub),
                   constraints=cons, options={'ftol': 1e-10, 'disp': False})

    # extract trim state and input and return
    trim_state = np.array([res.x[0:12]]).T
    trim_input = MsgDelta(elevator=res.x.item(12),
                          aileron=res.x.item(13),
                          rudder=res.x.item(14),
                          throttle=res.x.item(15))
    return trim_state, trim_input

def extract_state_input(x: types.NP_MAT) -> tuple[types.NP_MAT, MsgDelta]:
    """Extracts a state vector and control message from the aggregate vector

    Args:
        x: Euler state and inputs combined into a single vector

    Returns:
        states: Euler state vector
        delta: Control command
    """
    # Extract the state and input
    state = x[0:12]
    delta = MsgDelta(elevator=x.item(12),
                     aileron=x.item(13),
                     rudder=x.item(14),
                     throttle=x.item(15))
    return state, delta

def velocity_constraint(x: types.NP_MAT, Va_desired: float) -> float:
    """Returns the squared norm of the velocity vector - Va squared

    Args:
        x: Euler state and inputs combined into a single vector
            [pn, pe, pd, u, v, w, phi, theta, psi, p, q, r, delta_e, delta_a, delta_r, delta_t]
        Va_desired: Desired airspeed

    Returns:
        Va^2 - Va_desired^2
    """
    # Extract u, v, w from state vector (indices 3, 4, 5)
    u = x.item(3)
    v = x.item(4)
    w = x.item(5)
    # Calculate Va^2 and return difference from desired
    Va_squared = u**2 + v**2 + w**2
    return float(Va_squared - Va_desired**2)  # Ensure return type is float

def velocity_constraint_partial(x: types.NP_MAT) -> list[float]:
    """Defines the partial of the velocity constraint with respect to x

    Args:
        x: Euler state and inputs combined into a single vector
            [pn, pe, pd, u, v, w, phi, theta, psi, p, q, r, delta_e, delta_a, delta_r, delta_t]

    Returns:
        16 element list containing the partial of the constraint wrt x
    """
    # Extract u, v, w from state vector (indices 3, 4, 5)
    u = x.item(3)
    v = x.item(4)
    w = x.item(5)
    # Gradient of (u^2 + v^2 + w^2 - Va_desired^2) wrt x
    # d/du = 2*u, d/dv = 2*v, d/dw = 2*w, all others = 0
    return [0., 0., 0., 2.*u, 2.*v, 2.*w, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.]

def variable_bounds(state0: types.DynamicStateEuler, eps: float) -> tuple[list[float], list[float]]:
    """Define the upper and lower bounds of each the states and inputs as one vector.
       If an upper and lower bound is equivalent, then the upper bound is increased by eps to
       avoid a bug in scipy. If no bound exists then +/-np.inf is used.

    Each bound will be a list of the form
        [pn, pe, pd, u, v, w, phi, theta, psi, p, q, r, delta_e, delta_a, delta_r, delta_t]

    Args:
        state0: initial guess at the desired euler state
        eps: Small number (epsilon)

    Returns:
        lb: 16 element list defining the lower bound of each variable
        ub: 16 element list defining the upper bound of each variable
    """
    # Most state variables are bounded around their initial values
    # Some have fixed bounds
    lb = [
        # Position: pn, pe, pd
        state0.item(IND_EULER.NORTH), state0.item(IND_EULER.EAST), state0.item(IND_EULER.DOWN),
        # Velocity: u, v, w
        -np.inf, 0., -np.inf,
        # Euler angles: phi, theta, psi
        -np.pi/2, -np.pi/2+.1, state0.item(IND_EULER.PSI),
        # Angular rates: p, q, r
        0., 0., -np.inf,
        # Control inputs: delta_e, delta_a, delta_r, delta_t
        -np.pi/2, -np.pi/2, -np.pi/2, 0.
    ]
    
    ub = [
        # Position: pn, pe, pd
        state0.item(IND_EULER.NORTH) + eps, state0.item(IND_EULER.EAST) + eps, state0.item(IND_EULER.DOWN) + eps,
        # Velocity: u, v, w
        np.inf, eps, np.inf,
        # Euler angles: phi, theta, psi
        np.pi/2, np.pi/2-.1, state0.item(IND_EULER.PSI) + eps,
        # Angular rates: p, q, r
        eps, eps, np.inf,
        # Control inputs: delta_e, delta_a, delta_r, delta_t
        np.pi/2, np.pi/2, np.pi/2, 1.
    ]
    return lb, ub

def trim_objective_fun(x: types.NP_MAT, Va: float, gamma: float, R: float, psi_weight: float) -> float:
    """Calculates the cost on the trim trajectory being optimized using an Euler state representation

    Objective is the norm of the desired dynamics subtract the actual dynamics (except the x-y position variables)

    Args:
        x: current state and inputs combined into a single vector
            [pn, pe, pd, u, v, w, phi, theta, psi, p, q, r, delta_e, delta_a, delta_r, delta_t]
        Va: relative wind vector magnitude
        gamma: flight path angle
        R: radius - np.inf corresponds to a straight line

    Returns:
        J: resulting cost of the current parameters
    """
    # Extract the state and input
    state, delta = extract_state_input(x)

    # Ensure state is a column vector
    if state.ndim == 1:
        state = state.reshape(-1, 1)

    # Calculate the desired trim trajectory dynamics
    # For trim: pn_dot and pe_dot are not constrained
    # pd_dot = -Va * sin(gamma)
    # phi_dot = 0, theta_dot = 0, psi_dot = Va*cos(gamma)/R
    # u_dot = 0, v_dot = 0, w_dot = 0
    # p_dot = 0, q_dot = 0, r_dot = 0
    desired_state_dot = np.zeros((12, 1))
    desired_state_dot[IND_EULER.DOWN] = -Va * np.sin(gamma)
    desired_state_dot[IND_EULER.PSI] = Va * np.cos(gamma) / R

    # Convert Euler state to quaternion state for forces_moments and update_velocity_data
    state_quat = euler_state_to_quat_state(state)

    # Update velocity data needed for forces_moments
    # Returns (Va_actual, alpha, beta, wind_inertial)
    Va_actual, alpha, beta, _ = update_velocity_data(state_quat)

    # Calculate forces and moments
    # Note: forces_moments expects (state, delta, Va, beta, alpha)
    forces_moments_result = forces_moments(state_quat, delta, Va_actual, beta, alpha)

    # Calculate the dynamics based upon the current state and input (use euler derivatives)
    actual_state_dot = derivatives_euler(state, forces_moments_result)

    # Calculate the difference between the desired and actual (skip pn and pe)
    error = actual_state_dot - desired_state_dot

    # Calculate the square of the difference (neglecting pn and pe)
    # Weight psi_dot error more heavily
    J = error[IND_EULER.DOWN, 0]**2 + \
        error[IND_EULER.U, 0]**2 + \
        error[IND_EULER.V, 0]**2 + \
        error[IND_EULER.W, 0]**2 + \
        error[IND_EULER.PHI, 0]**2 + \
        error[IND_EULER.THETA, 0]**2 + \
        psi_weight * error[IND_EULER.PSI, 0]**2 + \
        error[IND_EULER.P, 0]**2 + \
        error[IND_EULER.Q, 0]**2 + \
        error[IND_EULER.R, 0]**2
    return float(J)
