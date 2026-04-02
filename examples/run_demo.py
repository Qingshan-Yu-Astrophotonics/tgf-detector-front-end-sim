from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path, env: dict[str, str]) -> None:
    print("Running:", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True, env=env)


def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    python_exe = sys.executable
    env = os.environ.copy()

    vendor_dir = repo_root / ".vendor"
    if vendor_dir.exists():
        existing = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = str(vendor_dir) if not existing else str(vendor_dir) + os.pathsep + existing

    synthetic_csv = repo_root / "examples" / "generated" / "edep_events.csv"
    output_root = repo_root / "examples" / "demo_outputs"

    run(
        [
            python_exe,
            "python_engine/generate_synthetic_edep.py",
            "--config",
            "config.yaml",
            "--output",
            str(synthetic_csv),
        ],
        cwd=repo_root,
        env=env,
    )
    run(
        [
            python_exe,
            "python_engine/pulse_synthesizer.py",
            "--config",
            "config.yaml",
            "--input",
            str(synthetic_csv),
            "--output-root",
            str(output_root),
            "--all-rates",
        ],
        cwd=repo_root,
        env=env,
    )

    print(f"Demo outputs are available in {output_root}")


if __name__ == "__main__":
    main()
