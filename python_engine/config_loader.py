from __future__ import annotations

from pathlib import Path
from typing import Any, Dict

import yaml


def load_config(config_path: str | Path) -> Dict[str, Any]:
    with Path(config_path).open("r", encoding="utf-8") as stream:
        config = yaml.safe_load(stream)

    if not isinstance(config, dict):
        raise ValueError("config.yaml must parse into a mapping")

    return config


def require_section(config: Dict[str, Any], section_name: str) -> Dict[str, Any]:
    section = config.get(section_name)
    if not isinstance(section, dict):
        raise KeyError(f"Missing required config section: {section_name}")
    return section
