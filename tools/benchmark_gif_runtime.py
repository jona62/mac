#!/usr/bin/env python3
from __future__ import annotations

import argparse
import statistics
import subprocess
import tempfile
import textwrap
import time
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAC_BINARY = ROOT / "build" / "mac"
BASELINES = {
    "png_still": 0.27,
    "gif_plain": 3.75,
    "timeline_simple": 6.78,
    "timeline_composed": 23.73,
}


@dataclass(frozen=True)
class BenchmarkCase:
    name: str
    extension: str
    script_template: str


CASES = [
    BenchmarkCase(
        name="png_still",
        extension="png",
        script_template="""
        @blank "PNG STILL" => "__OUTPUT__";
        """,
    ),
    BenchmarkCase(
        name="gif_plain",
        extension="gif",
        script_template="""
        gif loop {
            @blank 720x720 {{ top: "FRAME 1" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
            @blank 720x720 {{ top: "FRAME 2" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
            @blank 720x720 {{ top: "FRAME 3" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
            @blank 720x720 {{ top: "FRAME 4" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
            @blank 720x720 {{ top: "FRAME 5" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
            @blank 720x720 {{ top: "FRAME 6" center: "PLAIN GIF" bottom: "BENCHMARK" }} : 180ms
        } => "__OUTPUT__";
        """,
    ),
    BenchmarkCase(
        name="timeline_simple",
        extension="gif",
        script_template="""
        var scene_1 = @blank 720x720 {{ top: "SCENE 1" center: "TIMELINE" bottom: "SIMPLE" }};
        var scene_2 = @blank 720x720 {{ top: "SCENE 2" center: "TIMELINE" bottom: "SIMPLE" }};
        var scene_3 = @blank 720x720 {{ top: "SCENE 3" center: "TIMELINE" bottom: "SIMPLE" }};
        var scene_4 = @blank 720x720 {{ top: "SCENE 4" center: "TIMELINE" bottom: "SIMPLE" }};
        var scene_5 = @blank 720x720 {{ top: "SCENE 5" center: "TIMELINE" bottom: "SIMPLE" }};
        var scene_6 = @blank 720x720 {{ top: "SCENE 6" center: "TIMELINE" bottom: "SIMPLE" }};

        timeline loop {{
            scene_1 : 700ms
            --- crossfade 180ms ---
            scene_2 : 700ms
            --- crossfade 180ms ---
            scene_3 : 700ms
            --- crossfade 180ms ---
            scene_4 : 700ms
            --- crossfade 180ms ---
            scene_5 : 700ms
            --- crossfade 180ms ---
            scene_6 : 700ms
            --- crossfade 180ms ---
        }} => "__OUTPUT__";
        """,
    ),
    BenchmarkCase(
        name="timeline_composed",
        extension="gif",
        script_template="""
        var scene_1 = grid 2x2 {{
            @meme.girl_side_eye 360x360 {{ top: "EDITORIAL" center: "ONE" bottom: "A" }}
            @meme.guy_crying 360x360 {{ top: "EDITORIAL" center: "ONE" bottom: "B" }}
            @meme.idk_about_that 360x360 {{ top: "EDITORIAL" center: "ONE" bottom: "C" }}
            @meme.jordan_crying 360x360 {{ top: "EDITORIAL" center: "ONE" bottom: "D" }}
        }} |> pad(16) |> border(6) |> pixelate(4) |> contrast(1.8) |> noise(0.2);

        var scene_2 = grid 2x2 {{
            @meme.kid_crying 360x360 {{ top: "EDITORIAL" center: "TWO" bottom: "A" }}
            @meme.king_bach_stare 360x360 {{ top: "EDITORIAL" center: "TWO" bottom: "B" }}
            @meme.shrek_side_eye 360x360 {{ top: "EDITORIAL" center: "TWO" bottom: "C" }}
            @meme.window_despair 360x360 {{ top: "EDITORIAL" center: "TWO" bottom: "D" }}
        }} |> pad(16) |> border(6) |> pixelate(4) |> contrast(1.8) |> noise(0.2);

        var scene_3 = beside(
            @meme.shrek_smirk 360x720 {{ top: "SPLIT" center: "THREE" bottom: "LEFT" }},
            @meme.girl_side_eye 360x720 {{ top: "SPLIT" center: "THREE" bottom: "RIGHT" }}
        ) |> pad(20) |> border(8) |> blur(2) |> saturate(1.4);

        var scene_4 = stack(
            @meme.guy_crying 720x360 {{ top: "STACK" center: "FOUR" bottom: "TOP" }},
            @meme.idk_about_that 720x360 {{ top: "STACK" center: "FOUR" bottom: "BOTTOM" }}
        ) |> pad(20) |> border(8) |> blur(2) |> saturate(1.4);

        var scene_5 = grid 2x2 {{
            @meme.jordan_crying 360x360 {{ top: "EDITORIAL" center: "FIVE" bottom: "A" }}
            @meme.kid_crying 360x360 {{ top: "EDITORIAL" center: "FIVE" bottom: "B" }}
            @meme.king_bach_stare 360x360 {{ top: "EDITORIAL" center: "FIVE" bottom: "C" }}
            @meme.window_despair 360x360 {{ top: "EDITORIAL" center: "FIVE" bottom: "D" }}
        }} |> pad(16) |> border(6) |> glow(4) |> chromatic(2);

        var scene_6 = grid 2x2 {{
            @meme.shrek_side_eye 360x360 {{ top: "EDITORIAL" center: "SIX" bottom: "A" }}
            @meme.shrek_smirk 360x360 {{ top: "EDITORIAL" center: "SIX" bottom: "B" }}
            @meme.guy_crying 360x360 {{ top: "EDITORIAL" center: "SIX" bottom: "C" }}
            @meme.girl_side_eye 360x360 {{ top: "EDITORIAL" center: "SIX" bottom: "D" }}
        }} |> pad(16) |> border(6) |> glow(4) |> chromatic(2);

        timeline loop {{
            scene_1 : 650ms
            --- crossfade 180ms easeInOut ---
            scene_2 : 650ms
            --- crossfade 180ms easeInOut ---
            scene_3 : 650ms
            --- crossfade 180ms easeInOut ---
            scene_4 : 650ms
            --- crossfade 180ms easeInOut ---
            scene_5 : 650ms
            --- crossfade 180ms easeInOut ---
            scene_6 : 650ms
            --- crossfade 180ms easeInOut ---
        }} => "__OUTPUT__";
        """,
    ),
]


def render_case(case: BenchmarkCase, output_dir: Path, iteration: int) -> float:
    output_path = output_dir / f"{case.name}-{iteration}.{case.extension}"
    script = (
        textwrap.dedent(case.script_template)
        .strip()
        .replace("{{", "{")
        .replace("}}", "}")
        .replace("__OUTPUT__", str(output_path))
    )

    with tempfile.TemporaryDirectory(prefix=f"mac-bench-{case.name}-") as temp_dir:
        script_path = Path(temp_dir) / f"{case.name}.mac"
        script_path.write_text(script + "\n", encoding="utf-8")

        start = time.perf_counter()
        result = subprocess.run(
            [str(MAC_BINARY), str(script_path)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        elapsed = time.perf_counter() - start

    if result.returncode != 0:
        raise RuntimeError(f"{case.name} failed:\n{result.stderr or result.stdout}")
    if not output_path.exists():
        raise RuntimeError(f"{case.name} did not create {output_path}")
    return elapsed


def format_delta(current: float, baseline: float) -> str:
    if baseline <= 0:
        return "n/a"
    delta = (baseline - current) / baseline
    if delta >= 0:
        return f"{delta * 100:.1f}% faster"
    return f"{abs(delta) * 100:.1f}% slower"


def main() -> int:
    parser = argparse.ArgumentParser(description="Benchmark Mac GIF/timeline runtime cases.")
    parser.add_argument("--iterations", type=int, default=1, help="Runs per case (median reported).")
    args = parser.parse_args()

    if not MAC_BINARY.exists():
        raise SystemExit(f"Missing binary: {MAC_BINARY}. Run `cmake --build {ROOT / 'build'}` first.")

    with tempfile.TemporaryDirectory(prefix="mac-bench-output-") as output_dir_raw:
        output_dir = Path(output_dir_raw)
        print(f"Benchmark output dir: {output_dir}")
        print(f"Binary: {MAC_BINARY}")
        print()
        print(f"{'Case':<20} {'Current(s)':>10} {'Baseline(s)':>12} {'Delta':>16}")
        print("-" * 62)

        for case in CASES:
            runs = [render_case(case, output_dir, iteration) for iteration in range(args.iterations)]
            current = statistics.median(runs)
            baseline = BASELINES[case.name]
            print(f"{case.name:<20} {current:>10.3f} {baseline:>12.2f} {format_delta(current, baseline):>16}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
