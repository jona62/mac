# Tools

## Meme Asset Intake

Use `meme_asset_intake.py` before copying downloaded meme templates into
`assets/templates/meme`.

```bash
python3 tools/meme_asset_intake.py scan
python3 tools/meme_asset_intake.py check /tmp/meme-candidates
python3 tools/meme_asset_intake.py ingest /tmp/meme-candidates --dry-run
```

The tool reports exact duplicates with a SHA-256 hash of decoded RGBA pixels.
That ignores metadata, EXIF, comments, and container-level differences. It also
reports near duplicates with a perceptual dHash so a human reviewer or sub-agent
can decide whether a visually similar candidate is worth keeping.

For batch ingestion, provide reviewer-approved names with a slug map:

```json
{
  "02_pinterest_image_15c0b515_15c0b515c0b5.jpg": "distracted_boyfriend",
  "47_pinterest_image_e3a3cee3_e3a3cee3a3ce.jpg": "breaking_news_mic"
}
```

```bash
python3 tools/meme_asset_intake.py ingest /tmp/meme-candidates \
  --slug-map /tmp/meme-slugs.json \
  --dry-run
```

Keep reports, manifests, source HTML, and contact sheets outside
`assets/templates/meme`. The interpreter and analyzer treat image files in that
directory as meme templates.
