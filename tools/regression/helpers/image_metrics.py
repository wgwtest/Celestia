#!/usr/bin/env python3
import argparse
import json
import math
import os
import sys
import tempfile
from pathlib import Path

try:
    from PIL import Image, ImageDraw
    PIL_ERROR = None
except Exception as exc:  # pragma: no cover - depends on local environment
    Image = None
    ImageDraw = None
    PIL_ERROR = str(exc)


def write_json(path, payload):
    target = Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def unavailable_payload(path):
    return {
        "available": False,
        "path": str(path),
        "reason": "Pillow is not available: {}".format(PIL_ERROR),
    }


def pixel_data(image):
    if hasattr(image, "get_flattened_data"):
        return list(image.get_flattened_data())
    return list(image.getdata())


def dhash(image):
    small = image.convert("L").resize((9, 8))
    pixels = pixel_data(small)
    value = 0
    for row in range(8):
        for col in range(8):
            left = pixels[row * 9 + col]
            right = pixels[row * 9 + col + 1]
            value = (value << 1) | (1 if left > right else 0)
    return format(value, "016x")


def hamming_hex(left, right):
    if left is None or right is None:
        return None
    return bin(int(left, 16) ^ int(right, 16)).count("1")


def edge_density(image):
    gray = image.convert("L").resize((128, 128))
    pixels = pixel_data(gray)
    width, height = gray.size
    edges = 0
    samples = 0
    for y in range(height - 1):
        row = y * width
        next_row = (y + 1) * width
        for x in range(width - 1):
            current = pixels[row + x]
            dx = abs(current - pixels[row + x + 1])
            dy = abs(current - pixels[next_row + x])
            if dx + dy > 48:
                edges += 1
            samples += 1
    return edges / samples if samples else 0.0


def image_metrics(path):
    image_path = Path(path)
    if Image is None:
        return unavailable_payload(image_path)

    with Image.open(image_path) as opened:
        image = opened.convert("RGB")

    width, height = image.size
    pixels = pixel_data(image)

    total = len(pixels)
    if total == 0:
        raise ValueError("image has no pixels: {}".format(image_path))

    sums = [0, 0, 0]
    non_black = 0
    bright = 0
    colorful = 0
    for red, green, blue in pixels:
        sums[0] += red
        sums[1] += green
        sums[2] += blue
        if max(red, green, blue) > 10:
            non_black += 1
        if red + green + blue > 600:
            bright += 1
        if max(red, green, blue) - min(red, green, blue) > 30:
            colorful += 1

    return {
        "available": True,
        "path": str(image_path),
        "width": width,
        "height": height,
        "pixelCount": total,
        "averageColor": [round(value / total, 3) for value in sums],
        "nonBlackRatio": round(non_black / total, 6),
        "brightPixelRatio": round(bright / total, 6),
        "colorfulPixelRatio": round(colorful / total, 6),
        "edgeDensity": round(edge_density(image), 6),
        "dHash": dhash(image),
    }


def compare_images(baseline, current):
    baseline_metrics = image_metrics(baseline)
    current_metrics = image_metrics(current)
    payload = {
        "available": bool(baseline_metrics.get("available") and current_metrics.get("available")),
        "baseline": baseline_metrics,
        "current": current_metrics,
    }

    if payload["available"]:
        baseline_avg = baseline_metrics["averageColor"]
        current_avg = current_metrics["averageColor"]
        color_distance = math.sqrt(sum((baseline_avg[i] - current_avg[i]) ** 2 for i in range(3)))
        payload["comparison"] = {
            "sameDimensions": (
                baseline_metrics["width"] == current_metrics["width"]
                and baseline_metrics["height"] == current_metrics["height"]
            ),
            "averageColorDistance": round(color_distance, 3),
            "dHashHamming": hamming_hex(baseline_metrics.get("dHash"), current_metrics.get("dHash")),
            "nonBlackRatioDelta": round(
                current_metrics["nonBlackRatio"] - baseline_metrics["nonBlackRatio"], 6
            ),
            "edgeDensityDelta": round(
                current_metrics["edgeDensity"] - baseline_metrics["edgeDensity"], 6
            ),
        }
    else:
        payload["comparison"] = {
            "sameDimensions": None,
            "averageColorDistance": None,
            "dHashHamming": None,
            "nonBlackRatioDelta": None,
            "edgeDensityDelta": None,
        }

    return payload


def make_contact_sheet(entries, output):
    if Image is None:
        return unavailable_payload(output)

    thumbs = []
    thumb_width = 360
    label_height = 28
    padding = 10
    for label, path in entries:
        with Image.open(path) as opened:
            image = opened.convert("RGB")
            ratio = thumb_width / image.width
            thumb_height = max(1, int(image.height * ratio))
            thumb = image.resize((thumb_width, thumb_height))
            thumbs.append((label, thumb))

    if not thumbs:
        raise ValueError("no contact-sheet entries were provided")

    sheet_width = thumb_width + padding * 2
    sheet_height = sum(thumb.height + label_height + padding for _, thumb in thumbs) + padding
    sheet = Image.new("RGB", (sheet_width, sheet_height), (18, 18, 18))
    draw = ImageDraw.Draw(sheet)
    y = padding
    for label, thumb in thumbs:
        draw.text((padding, y), label, fill=(235, 235, 235))
        y += label_height
        sheet.paste(thumb, (padding, y))
        y += thumb.height + padding

    output_path = Path(output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)
    return {
        "available": True,
        "path": str(output_path),
        "entries": len(entries),
        "width": sheet.width,
        "height": sheet.height,
    }


def run_self_test():
    if Image is None:
        payload = unavailable_payload("self-test")
        if not payload["reason"]:
            raise AssertionError("fallback reason is empty")
        print("image_metrics self-test passed")
        return 0

    with tempfile.TemporaryDirectory() as temp_dir:
        left = Path(temp_dir) / "left.png"
        right = Path(temp_dir) / "right.png"
        Image.new("RGB", (32, 24), (40, 80, 160)).save(left)
        Image.new("RGB", (32, 24), (45, 75, 150)).save(right)
        metrics = image_metrics(left)
        if not metrics["available"]:
            raise AssertionError("metrics unexpectedly unavailable")
        if metrics["width"] != 32 or metrics["height"] != 24:
            raise AssertionError("wrong test image dimensions")
        comparison = compare_images(left, right)
        if comparison["comparison"]["sameDimensions"] is not True:
            raise AssertionError("sameDimensions comparison failed")
        sheet = make_contact_sheet([("left", left), ("right", right)], Path(temp_dir) / "sheet.png")
        if not sheet["available"]:
            raise AssertionError("contact sheet failed")

    print("image_metrics self-test passed")
    return 0


def parse_entry(value):
    if "=" not in value:
        raise argparse.ArgumentTypeError("sheet entry must use label=path")
    label, path = value.split("=", 1)
    if not label:
        raise argparse.ArgumentTypeError("sheet entry label is empty")
    return label, path


def main(argv):
    parser = argparse.ArgumentParser(description="Celestia regression image metrics")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--image")
    parser.add_argument("--baseline")
    parser.add_argument("--current")
    parser.add_argument("--json")
    parser.add_argument("--contact-sheet")
    parser.add_argument("--sheet-entry", action="append", type=parse_entry, default=[])
    args = parser.parse_args(argv)

    if args.self_test:
        return run_self_test()

    if args.contact_sheet:
        payload = make_contact_sheet(args.sheet_entry, args.contact_sheet)
    elif args.baseline and args.current:
        payload = compare_images(args.baseline, args.current)
    elif args.image:
        payload = image_metrics(args.image)
    else:
        parser.error("expected --self-test, --image, --baseline/--current, or --contact-sheet")

    if args.json:
        write_json(args.json, payload)
    else:
        print(json.dumps(payload, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
