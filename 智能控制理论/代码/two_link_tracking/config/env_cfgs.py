"""Two-link arm trajectory-tracking environment configuration."""

from __future__ import annotations

import copy
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

import mujoco
import numpy as np
import torch
from mjlab.actuator.xml_actuator import XmlActuatorCfg
from mjlab.entity import Entity, EntityArticulationInfoCfg, EntityCfg
from mjlab.envs import ManagerBasedRlEnvCfg
from mjlab.envs import mdp as envs_mdp
from mjlab.envs.mdp import (
  action_rate_l2,
  dr,
  joint_pos_rel,
  joint_vel_rel,
  time_out,
)
from mjlab.envs.mdp.actions.actions import JointEffortAction, JointEffortActionCfg
from mjlab.managers.action_manager import ActionTermCfg
from mjlab.managers.command_manager import CommandTerm, CommandTermCfg
from mjlab.managers.event_manager import EventTermCfg
from mjlab.managers.metrics_manager import MetricsTermCfg
from mjlab.managers.observation_manager import ObservationGroupCfg, ObservationTermCfg
from mjlab.managers.reward_manager import RewardTermCfg
from mjlab.managers.scene_entity_config import SceneEntityCfg
from mjlab.managers.termination_manager import TerminationTermCfg
from mjlab.scene import SceneCfg
from mjlab.sim import MujocoCfg, SimulationCfg
from mjlab.terrains import TerrainEntityCfg
from mjlab.utils.noise import UniformNoiseCfg as Unoise
from mjlab.viewer import ViewerConfig

if TYPE_CHECKING:
  from mjlab.envs import ManagerBasedRlEnv

_XML: Path = Path(__file__).parents[1] / "xmls" / "two_link_arm.xml"
_ARM_CFG = SceneEntityCfg(
  "two_link",
  joint_names=("joint1", "joint2"),
  actuator_names=("joint1", "joint2"),
  preserve_order=True,
)
_DESIRED_Q2 = 0.5
_TRAJECTORY_W = 2.0
_PHASE_OFFSET_KEY = "two_link_phase_offset"


def _get_spec() -> mujoco.MjSpec:
  return mujoco.MjSpec.from_file(str(_XML))


_ARTICULATION = EntityArticulationInfoCfg(
  actuators=(
    XmlActuatorCfg(target_names_expr=("joint1",)),
    XmlActuatorCfg(target_names_expr=("joint2",)),
  ),
)

_INIT = EntityCfg.InitialStateCfg(
  pos=(0.0, 0.0, 0.0),
  joint_pos={"joint1": 1.0, "joint2": _DESIRED_Q2},
  joint_vel={".*": 0.0},
)


def _get_two_link_cfg() -> EntityCfg:
  return EntityCfg(
    spec_fn=_get_spec,
    articulation=_ARTICULATION,
    init_state=_INIT,
  )


def _episode_time(env: ManagerBasedRlEnv) -> torch.Tensor:
  return env.episode_length_buf.float() * env.step_dt


def _phase_offset(env: ManagerBasedRlEnv) -> torch.Tensor:
  extras = getattr(env, "extras", None)
  if not isinstance(extras, dict):
    return torch.zeros_like(env.episode_length_buf, dtype=torch.float32)

  phase = extras.get(_PHASE_OFFSET_KEY)
  target_shape = env.episode_length_buf.shape
  if not isinstance(phase, torch.Tensor) or phase.shape != target_shape:
    phase = torch.zeros(target_shape, device=env.device, dtype=torch.float32)
    extras[_PHASE_OFFSET_KEY] = phase
  return phase


def _trajectory_argument(env: ManagerBasedRlEnv) -> torch.Tensor:
  return _TRAJECTORY_W * _episode_time(env) + _phase_offset(env)


def desired_joint_pos(env: ManagerBasedRlEnv) -> torch.Tensor:
  arg = _trajectory_argument(env)
  return torch.stack(
    (torch.cos(arg), torch.full_like(arg, _DESIRED_Q2)),
    dim=-1,
  )


def desired_joint_vel(env: ManagerBasedRlEnv) -> torch.Tensor:
  arg = _trajectory_argument(env)
  return torch.stack(
    (-_TRAJECTORY_W * torch.sin(arg), torch.zeros_like(arg)),
    dim=-1,
  )


def desired_joint_acc(env: ManagerBasedRlEnv) -> torch.Tensor:
  arg = _trajectory_argument(env)
  return torch.stack(
    (-_TRAJECTORY_W**2 * torch.cos(arg), torch.zeros_like(arg)),
    dim=-1,
  )


def trajectory_phase(env: ManagerBasedRlEnv) -> torch.Tensor:
  arg = _trajectory_argument(env)
  return torch.stack((torch.sin(arg), torch.cos(arg)), dim=-1)


def disturbance_torque(env: ManagerBasedRlEnv) -> torch.Tensor:
  t = _episode_time(env)
  return torch.stack(
    (
      0.5 * (1.0 - torch.exp(-t)),
      0.5 * torch.sin(t) + torch.cos(4.0 * t),
    ),
    dim=-1,
  )


def joint_pos_error(
  env: ManagerBasedRlEnv,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  asset: Entity = env.scene[asset_cfg.name]
  return asset.data.joint_pos[:, asset_cfg.joint_ids] - desired_joint_pos(env)


def joint_vel_error(
  env: ManagerBasedRlEnv,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  asset: Entity = env.scene[asset_cfg.name]
  return asset.data.joint_vel[:, asset_cfg.joint_ids] - desired_joint_vel(env)


def joint_pos_tracking_reward(
  env: ManagerBasedRlEnv,
  std: float,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  err = joint_pos_error(env, asset_cfg)
  return torch.exp(-torch.mean(torch.square(err / std), dim=-1))


def joint_vel_tracking_reward(
  env: ManagerBasedRlEnv,
  std: float,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  err = joint_vel_error(env, asset_cfg)
  return torch.exp(-torch.mean(torch.square(err / std), dim=-1))


def joint_pos_error_l2(
  env: ManagerBasedRlEnv,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  return torch.sum(torch.square(joint_pos_error(env, asset_cfg)), dim=-1)


def joint_vel_error_l2(
  env: ManagerBasedRlEnv,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  return torch.sum(torch.square(joint_vel_error(env, asset_cfg)), dim=-1)


def joint_position_metric(
  env: ManagerBasedRlEnv,
  joint_local_idx: int,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  asset: Entity = env.scene[asset_cfg.name]
  joint_ids = asset_cfg.joint_ids
  if isinstance(joint_ids, slice):
    return asset.data.joint_pos[:, joint_local_idx]
  return asset.data.joint_pos[:, joint_ids[joint_local_idx]]


def joint_tracking_failed(
  env: ManagerBasedRlEnv,
  pos_threshold: float,
  vel_threshold: float,
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> torch.Tensor:
  pos_err = torch.abs(joint_pos_error(env, asset_cfg))
  vel_err = torch.abs(joint_vel_error(env, asset_cfg))
  pos_failed = torch.any(pos_err > pos_threshold, dim=-1)
  vel_failed = torch.any(vel_err > vel_threshold, dim=-1)
  return pos_failed | vel_failed


def reset_to_reference_phase(
  env: ManagerBasedRlEnv,
  env_ids: torch.Tensor | None,
  phase_range: tuple[float, float],
  position_noise_range: tuple[float, float],
  velocity_noise_range: tuple[float, float],
  asset_cfg: SceneEntityCfg = _ARM_CFG,
) -> None:
  if env_ids is None:
    env_ids = torch.arange(env.num_envs, device=env.device, dtype=torch.int)

  phase_offset = _phase_offset(env)
  phase_offset[env_ids] = torch.empty(len(env_ids), device=env.device).uniform_(
    *phase_range
  )

  asset: Entity = env.scene[asset_cfg.name]
  phase = phase_offset[env_ids]
  joint_pos = torch.stack(
    (torch.cos(phase), torch.full_like(phase, _DESIRED_Q2)),
    dim=-1,
  )
  joint_vel = torch.stack(
    (-_TRAJECTORY_W * torch.sin(phase), torch.zeros_like(phase)),
    dim=-1,
  )

  joint_pos += torch.empty_like(joint_pos).uniform_(*position_noise_range)
  joint_vel += torch.empty_like(joint_vel).uniform_(*velocity_noise_range)
  asset.write_joint_state_to_sim(joint_pos, joint_vel, env_ids=env_ids)


@dataclass(kw_only=True)
class DisturbedJointEffortActionCfg(JointEffortActionCfg):
  enable_disturbance: bool = False

  def build(self, env: ManagerBasedRlEnv) -> DisturbedJointEffortAction:
    return DisturbedJointEffortAction(self, env)


class DisturbedJointEffortAction(JointEffortAction):
  cfg: DisturbedJointEffortActionCfg

  def apply_actions(self) -> None:
    effort = self._processed_actions
    if self.cfg.enable_disturbance:
      effort = effort + disturbance_torque(self._env)
    self._entity.set_joint_effort_target(effort, joint_ids=self._target_ids)


class DesiredTrajectoryGhostCommand(CommandTerm):
  cfg: "DesiredTrajectoryGhostCommandCfg"

  def __init__(self, cfg: "DesiredTrajectoryGhostCommandCfg", env: ManagerBasedRlEnv):
    super().__init__(cfg, env)
    self._entity: Entity = env.scene[cfg.entity_name]
    self._joint_q_adr = self._entity.indexing.joint_q_adr.cpu().numpy()
    self._ghost_model: mujoco.MjModel | None = None
    self._ghost_color = np.array(cfg.viz.ghost_color, dtype=np.float32)
    self._command = torch.zeros(self.num_envs, 2, device=self.device)

  @property
  def command(self) -> torch.Tensor:
    return self._command

  def _resample_command(self, env_ids: torch.Tensor) -> None:
    del env_ids

  def _update_command(self) -> None:
    self._command = desired_joint_pos(self._env)

  def _update_metrics(self) -> None:
    pass

  def _debug_vis_impl(self, visualizer) -> None:
    env_indices = visualizer.get_env_indices(self.num_envs)
    if not env_indices:
      return

    if self._ghost_model is None:
      self._ghost_model = copy.deepcopy(self._env.sim.mj_model)
      for geom_id in range(self._ghost_model.ngeom):
        if (
          self._ghost_model.geom_contype[geom_id] != 0
          or self._ghost_model.geom_conaffinity[geom_id] != 0
        ):
          self._ghost_model.geom_rgba[geom_id, 3] = 0
        else:
          self._ghost_model.geom_rgba[geom_id] = self._ghost_color

    q_des = self._command.detach().cpu().numpy()
    for env_idx in env_indices:
      qpos = np.zeros(self._env.sim.mj_model.nq)
      qpos[self._joint_q_adr] = q_des[env_idx]
      visualizer.add_ghost_mesh(
        qpos,
        model=self._ghost_model,
        alpha=self.cfg.viz.alpha,
        label=f"two_link_desired_ghost_{env_idx}",
      )


@dataclass(kw_only=True)
class DesiredTrajectoryGhostCommandCfg(CommandTermCfg):
  @dataclass
  class VizCfg:
    ghost_color: tuple[float, float, float, float] = (0.0, 1.0, 0.25, 0.42)
    alpha: float = 0.42

  entity_name: str
  viz: VizCfg = field(default_factory=VizCfg)

  def build(self, env: ManagerBasedRlEnv) -> DesiredTrajectoryGhostCommand:
    return DesiredTrajectoryGhostCommand(self, env)


def two_link_tracking_env_cfg(
  play: bool = False,
  enable_reference_disturbance: bool | None = None,
) -> ManagerBasedRlEnvCfg:
  if enable_reference_disturbance is None:
    enable_reference_disturbance = play

  actor_terms = {
    "joint_pos": ObservationTermCfg(
      func=joint_pos_rel,
      params={"asset_cfg": _ARM_CFG},
      noise=Unoise(n_min=-0.005, n_max=0.005),
    ),
    "joint_vel": ObservationTermCfg(
      func=joint_vel_rel,
      params={"asset_cfg": _ARM_CFG},
      noise=Unoise(n_min=-0.02, n_max=0.02),
    ),
    "desired_joint_pos": ObservationTermCfg(func=desired_joint_pos),
    "desired_joint_vel": ObservationTermCfg(func=desired_joint_vel),
    "desired_joint_acc": ObservationTermCfg(func=desired_joint_acc),
    "phase": ObservationTermCfg(func=trajectory_phase),
  }
  critic_terms = {
    **actor_terms,
    "joint_pos_error": ObservationTermCfg(
      func=joint_pos_error,
      params={"asset_cfg": _ARM_CFG},
    ),
    "joint_vel_error": ObservationTermCfg(
      func=joint_vel_error,
      params={"asset_cfg": _ARM_CFG},
    ),
  }

  observations = {
    "actor": ObservationGroupCfg(
      terms=actor_terms,
      concatenate_terms=True,
      enable_corruption=True,
    ),
    "critic": ObservationGroupCfg(
      terms=critic_terms,
      concatenate_terms=True,
      enable_corruption=False,
    ),
  }

  actions: dict[str, ActionTermCfg] = {
    "joint_effort": DisturbedJointEffortActionCfg(
      entity_name="two_link",
      actuator_names=("joint1", "joint2"),
      scale=12.0,
      clip={".*": (-25.0, 25.0)},
      enable_disturbance=enable_reference_disturbance,
      preserve_order=True,
    )
  }

  events = {
    "reset_reference_phase": EventTermCfg(
      func=reset_to_reference_phase,
      mode="reset",
      params={
        "phase_range": (0.0, 2.0 * np.pi),
        "position_noise_range": (-0.05, 0.05),
        "velocity_noise_range": (-0.1, 0.1),
        "asset_cfg": _ARM_CFG,
      },
    ),
    "encoder_bias": EventTermCfg(
      mode="startup",
      func=dr.encoder_bias,
      params={
        "asset_cfg": _ARM_CFG,
        "bias_range": (-0.01, 0.01),
      },
    ),
    "link_com": EventTermCfg(
      mode="startup",
      func=dr.body_com_offset,
      params={
        "asset_cfg": SceneEntityCfg("two_link", body_names=("link1", "link2")),
        "operation": "add",
        "ranges": {
          0: (-0.015, 0.015),
          1: (-0.01, 0.01),
          2: (-0.015, 0.015),
        },
      },
    ),
    "link_wrench_disturbance": EventTermCfg(
      mode="interval",
      interval_range_s=(0.8, 2.0),
      func=envs_mdp.apply_external_force_torque,
      params={
        "asset_cfg": SceneEntityCfg("two_link", body_names=("link2",)),
        "force_range": (-6.0, 6.0),
        "torque_range": (-0.4, 0.4),
      },
    ),
  }

  rewards = {
    "joint_pos_tracking": RewardTermCfg(
      func=joint_pos_tracking_reward,
      weight=10.0,
      params={"std": 0.12, "asset_cfg": _ARM_CFG},
    ),
    "joint_vel_tracking": RewardTermCfg(
      func=joint_vel_tracking_reward,
      weight=2.0,
      params={"std": 0.8, "asset_cfg": _ARM_CFG},
    ),
    "action_rate_l2": RewardTermCfg(func=action_rate_l2, weight=-0.02),
  }

  metrics = {
    "joint1_pos": MetricsTermCfg(
      func=joint_position_metric,
      params={"joint_local_idx": 0, "asset_cfg": _ARM_CFG},
    ),
    "joint2_pos": MetricsTermCfg(
      func=joint_position_metric,
      params={"joint_local_idx": 1, "asset_cfg": _ARM_CFG},
    ),
    "joint_pos_error_l2": MetricsTermCfg(
      func=joint_pos_error_l2,
      params={"asset_cfg": _ARM_CFG},
    ),
    "joint_vel_error_l2": MetricsTermCfg(
      func=joint_vel_error_l2,
      params={"asset_cfg": _ARM_CFG},
    ),
  }

  cfg = ManagerBasedRlEnvCfg(
    scene=SceneCfg(
      terrain=TerrainEntityCfg(terrain_type="plane"),
      entities={"two_link": _get_two_link_cfg()},
      num_envs=1,
      env_spacing=2.0,
    ),
    observations=observations,
    actions=actions,
    commands={
      "desired_trajectory_ghost": DesiredTrajectoryGhostCommandCfg(
        entity_name="two_link",
        resampling_time_range=(1.0e9, 1.0e9),
        debug_vis=play,
      )
    },
    events=events,
    rewards=rewards,
    terminations={
      "time_out": TerminationTermCfg(func=time_out, time_out=True),
      "tracking_failed": TerminationTermCfg(
        func=joint_tracking_failed,
        params={
          "pos_threshold": 1.2,
          "vel_threshold": 8.0,
          "asset_cfg": _ARM_CFG,
        },
      ),
    },
    metrics=metrics,
    viewer=ViewerConfig(
      origin_type=ViewerConfig.OriginType.ASSET_BODY,
      entity_name="two_link",
      body_name="base",
      distance=2.2,
      elevation=0.0,
      azimuth=90.0,
    ),
    sim=SimulationCfg(
      mujoco=MujocoCfg(
        timestep=0.002,
        iterations=10,
        ls_iterations=20,
        disableflags=("contact",),
      ),
    ),
    decimation=10,
    episode_length_s=8.0,
  )

  if play:
    cfg.episode_length_s = int(1e9)
    cfg.observations["actor"].enable_corruption = False
    cfg.terminations.pop("tracking_failed", None)
    cfg.events.pop("link_wrench_disturbance", None)

  return cfg
