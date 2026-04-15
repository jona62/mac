import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from webapp import server


class StudioServerTests(unittest.TestCase):
    def test_single_png_document_uses_scene_export(self) -> None:
        document = server.normalize_document(
            {
                "canvas": {"width": 720, "height": 720},
                "output": {"format": "png"},
                "scenes": [
                    {
                        "durationMs": 900,
                        "layout": {"kind": "single", "padding": 0, "border": 0, "effect": "none"},
                        "slots": [
                            {
                                "templateId": "blank",
                                "text": {"center": "HELLO"},
                                "style": {"preset": "cinematic"},
                            }
                        ],
                    }
                ],
            }
        )

        bundle = server.build_script_bundle(document, Path("/tmp/output.png"))

        self.assertIn('style cinematic {', bundle["documentScript"])
        self.assertIn('@blank 720x720 cinematic {', bundle["documentScript"])
        self.assertIn('center: "HELLO"', bundle["documentScript"])
        self.assertIn('scene_1 => "studio-output.png";', bundle["documentScript"])

    def test_multi_scene_document_requires_gif(self) -> None:
        with self.assertRaisesRegex(ValueError, "PNG export only supports a single scene"):
            server.normalize_document(
                {
                    "canvas": {"width": 720, "height": 720},
                    "output": {"format": "png"},
                    "scenes": [
                        {
                            "durationMs": 300,
                            "layout": {"kind": "single", "effect": "none"},
                            "slots": [{"templateId": "blank", "text": {}, "style": {}}],
                        },
                        {
                            "durationMs": 300,
                            "layout": {"kind": "single", "effect": "none"},
                            "slots": [{"templateId": "blank", "text": {}, "style": {}}],
                        },
                    ],
                }
            )

    def test_layout_slot_count_is_validated(self) -> None:
        with self.assertRaisesRegex(ValueError, "expects 4 slot"):
            server.normalize_document(
                {
                    "canvas": {"width": 720, "height": 720},
                    "output": {"format": "png"},
                    "scenes": [
                        {
                            "durationMs": 700,
                            "layout": {"kind": "grid2x2", "effect": "none"},
                            "slots": [
                                {"templateId": "blank", "text": {}, "style": {}},
                                {"templateId": "blank", "text": {}, "style": {}},
                            ],
                        }
                    ],
                }
            )

    def test_scene_effect_padding_and_border_order(self) -> None:
        document = server.normalize_document(
            {
                "canvas": {"width": 900, "height": 900},
                "output": {"format": "png"},
                "scenes": [
                    {
                        "durationMs": 950,
                        "layout": {"kind": "grid2x2", "padding": 8, "border": 2, "effect": "glitch"},
                        "slots": [
                            {"templateId": "blank", "text": {"center": "A"}, "style": {}},
                            {"templateId": "blank", "text": {"center": "B"}, "style": {}},
                            {"templateId": "blank", "text": {"center": "C"}, "style": {}},
                            {"templateId": "blank", "text": {"center": "D"}, "style": {}},
                        ],
                    }
                ],
            }
        )

        bundle = server.build_script_bundle(document, Path("/tmp/grid.png"))

        self.assertIn("grid 2x2 {", bundle["documentScript"])
        self.assertIn("effect fx_glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);", bundle["documentScript"])
        self.assertIn("var scene_1 = grid 2x2 {", bundle["documentScript"])
        self.assertIn("} |> pad(8) |> border(2) |> fx_glitch;", bundle["documentScript"])

    def test_custom_font_size_serializes_as_number(self) -> None:
        document = server.normalize_document(
            {
                "canvas": {"width": 720, "height": 720},
                "output": {"format": "png"},
                "scenes": [
                    {
                        "durationMs": 800,
                        "layout": {"kind": "single", "effect": "none"},
                        "slots": [
                            {
                                "templateId": "blank",
                                "text": {"top": "LOUD"},
                                "style": {"fontSizeMode": "custom", "fontSizePx": 88, "color": "#FF0000"},
                            }
                        ],
                    }
                ],
            }
        )

        bundle = server.build_script_bundle(document, Path("/tmp/font.png"))

        self.assertIn("style scene_1_slot_1_style {", bundle["documentScript"])
        self.assertIn("fontSize: 88", bundle["documentScript"])
        self.assertIn('color: "#FF0000"', bundle["documentScript"])

    def test_preview_bundle_keeps_document_script_authoritative(self) -> None:
        document = server.normalize_document(
            {
                "canvas": {"width": 720, "height": 720},
                "output": {"format": "gif"},
                "previewSceneIndex": 1,
                "scenes": [
                    {
                        "durationMs": 320,
                        "layout": {"kind": "single", "effect": "none"},
                        "slots": [{"templateId": "blank", "text": {"center": "ONE"}, "style": {}}],
                    },
                    {
                        "durationMs": 440,
                        "layout": {"kind": "single", "effect": "none"},
                        "slots": [{"templateId": "blank", "text": {"center": "TWO"}, "style": {}}],
                    },
                ],
            }
        )

        bundle = server.build_script_bundle(document, Path("/tmp/preview.png"), preview_scene_index=1)

        self.assertIn('} => "studio-output.gif";', bundle["documentScript"])
        self.assertIn('scene_2 => "/tmp/preview.png";', bundle["runScript"])


if __name__ == "__main__":
    unittest.main()
