from mjlab.tasks.registry import register_mjlab_task

from .env_cfgs import two_link_tracking_env_cfg
from .rl_cfg import two_link_tracking_ppo_runner_cfg

register_mjlab_task(
  task_id="Mjlab-TwoLink-Track",
  env_cfg=two_link_tracking_env_cfg(),
  play_env_cfg=two_link_tracking_env_cfg(play=True),
  rl_cfg=two_link_tracking_ppo_runner_cfg(),
)

