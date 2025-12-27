import os
import argparse
from PIL import Image
import numpy as np

def crop_to_alpha_bbox(img, padding=0):
    alpha = np.array(img.split()[-1])
    ys, xs = np.where(alpha > 0)
    if len(xs) == 0 or len(ys) == 0:
        return None  # 全透明
    left, right = xs.min(), xs.max()
    top, bottom = ys.min(), ys.max()
    # 加 padding 并裁剪到图片范围
    left = max(left - padding, 0)
    right = min(right + padding, img.width - 1)
    top = max(top - padding, 0)
    bottom = min(bottom + padding, img.height - 1)
    return img.crop((left, top, right + 1, bottom + 1))

def process_dir(input_dir, padding):
    output_dir = os.path.join(input_dir, "output")
    os.makedirs(output_dir, exist_ok=True)
    for fname in os.listdir(input_dir):
        if fname.lower().endswith('.png'):
            path = os.path.join(input_dir, fname)
            img = Image.open(path).convert('RGBA')
            cropped = crop_to_alpha_bbox(img, padding)
            if cropped:
                cropped.save(os.path.join(output_dir, fname))
                print(f"Processed: {fname}")
            else:
                print(f"Skipped (all transparent): {fname}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Clip PNGs to alpha bounding box.")
    parser.add_argument("dir", help="Input directory containing PNG files")
    parser.add_argument("padding", type=int, help="Padding around bounding box (pixels)")
    args = parser.parse_args()
    process_dir(args.dir, args.padding)