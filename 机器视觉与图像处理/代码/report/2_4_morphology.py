import sys
from pathlib import Path

import cv2
import numpy as np


def resolve_input_path() -> Path:
    if len(sys.argv) > 1:
        return Path(sys.argv[1])

    candidates = [
        Path(r"D:\project\Personal\COURSES\CV\report\cracks.jpg"),
        Path("/mnt/d/project/Personal/COURSES/CV/report/cracks.jpg"),
        Path(__file__).resolve().parent / "cracks.jpg",
    ]
    for path in candidates:
        if path.exists():
            return path
    return candidates[-1]


def remove_small_components(binary: np.ndarray, min_area: int) -> np.ndarray:
    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(binary, connectivity=8)
    cleaned = np.zeros_like(binary)
    for label in range(1, num_labels):
        if stats[label, cv2.CC_STAT_AREA] >= min_area:
            cleaned[labels == label] = 255
    return cleaned


def main() -> None:
    input_path = resolve_input_path()
    image = cv2.imread(str(input_path))
    if image is None:
        raise SystemExit(f"Failed to read image: {input_path}")

    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    gray = cv2.medianBlur(gray, 3)
    binary = cv2.adaptiveThreshold(
        gray,
        255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY_INV,
        41,
        5,
    )

    open_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
    close_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (9, 3))
    opened = cv2.morphologyEx(binary, cv2.MORPH_OPEN, open_kernel, iterations=1)
    cleaned = remove_small_components(opened, min_area=120)
    closed = cv2.morphologyEx(cleaned, cv2.MORPH_CLOSE, close_kernel, iterations=1)

    opening_path = input_path.parent / "cracks_2_4_opening.jpg"
    closing_path = input_path.parent / "cracks_2_4_closing.jpg"
    if not cv2.imwrite(str(opening_path), cleaned):
        raise SystemExit(f"Failed to write image: {opening_path}")
    if not cv2.imwrite(str(closing_path), closed):
        raise SystemExit(f"Failed to write image: {closing_path}")

    print(f"Saved: {opening_path}")
    print(f"Saved: {closing_path}")


if __name__ == "__main__":
    main()
