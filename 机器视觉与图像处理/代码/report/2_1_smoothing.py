import sys
from pathlib import Path

import cv2


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


def main() -> None:
    input_path = resolve_input_path()
    image = cv2.imread(str(input_path))
    if image is None:
        raise SystemExit(f"Failed to read image: {input_path}")

    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    output_path = input_path.parent / "cracks_2_1_smooth.jpg"
    if not cv2.imwrite(str(output_path), blurred):
        raise SystemExit(f"Failed to write image: {output_path}")

    print(f"Saved: {output_path}")


if __name__ == "__main__":
    main()
