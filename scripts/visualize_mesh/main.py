import os
import argparse
import numpy as np
import trimesh
import pyrender
from PIL import Image


def parse_args():
    p = argparse.ArgumentParser(description="Render OBJ files to PNG using pyrender")
    p.add_argument(
        "--obj-dir",
        "-i",
        required=True,
        help="Directory containing .obj files",
    )
    p.add_argument("--out-dir", "-o", required=True, help="Output directory for PNGs")
    p.add_argument(
        "--img-size",
        "-s",
        type=int,
        default=512,
        help="Output image size (square)",
    )
    p.add_argument(
        "--model-scale",
        "-m",
        type=float,
        default=1.0,
        help="Scale multiplier applied to normalized model (default: 1.5)",
    )
    p.add_argument(
        "--cam-distance",
        "-d",
        type=float,
        default=1.5,
        help="Camera distance along +Z from origin (default: 1.5)",
    )
    return p.parse_args()


def normalize_mesh(mesh: trimesh.Trimesh):
    mesh = mesh.copy()
    mesh.apply_translation(-mesh.bounding_box.centroid)
    scale = 1.0 / mesh.bounding_box.extents.max()
    mesh.apply_scale(scale)
    return mesh


def main():
    args = parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    # ========= 固定相机参数 =========
    camera = pyrender.PerspectiveCamera(yfov=np.pi / 3.0, aspectRatio=1.0)

    # 相机位置（世界坐标） — 使用 look_at 构造更可靠的相机位姿
    def look_at(eye, target, up=np.array([0.0, 1.0, 0.0])):
        eye = np.array(eye, dtype=float)
        target = np.array(target, dtype=float)
        up = np.array(up, dtype=float)
        forward = target - eye
        forward /= np.linalg.norm(forward)
        right = np.cross(forward, up)
        right /= np.linalg.norm(right)
        true_up = np.cross(right, forward)
        mat = np.eye(4, dtype=float)
        mat[0, :3] = right
        mat[1, :3] = true_up
        mat[2, :3] = -forward
        mat[:3, 3] = eye
        return mat

    cam_pose = look_at(
        eye=[0.0, 0.0, args.cam_distance], target=[0.0, 0.0, 0.0], up=[0.0, 1.0, 0.0]
    )

    # ========= 光照 =========
    light = pyrender.DirectionalLight(color=np.ones(3), intensity=3.0)
    # 额外再加一盏柔和的方向光，避免单光源导致过曝或朝向问题
    light2 = pyrender.DirectionalLight(color=np.ones(3), intensity=1.0)
    light2_pose = look_at(
        eye=[2.0, 2.0, 2.0], target=[0.0, 0.0, 0.0], up=[0.0, 1.0, 0.0]
    )

    renderer = pyrender.OffscreenRenderer(
        viewport_width=args.img_size, viewport_height=args.img_size
    )

    # ========= 主循环 =========
    for fname in sorted(os.listdir(args.obj_dir)):
        if not fname.endswith(".obj"):
            continue

        path = os.path.join(args.obj_dir, fname)
        mesh = trimesh.load(path, force="mesh")

        if isinstance(mesh, trimesh.Scene):
            mesh = trimesh.util.concatenate(mesh.dump())

        mesh = normalize_mesh(mesh)
        # 放大模型以便在图像中更大显示
        if hasattr(args, "model_scale") and args.model_scale != 1.0:
            mesh.apply_scale(float(args.model_scale))

        material = pyrender.MetallicRoughnessMaterial(
            baseColorFactor=(0.8, 0.8, 0.8, 1.0),
            metallicFactor=0.0,
            roughnessFactor=0.8,
        )

        render_mesh = pyrender.Mesh.from_trimesh(mesh, material=material)

        scene = pyrender.Scene(bg_color=[255, 255, 255, 255])
        scene.add(render_mesh)

        scene.add(camera, pose=cam_pose)
        scene.add(light, pose=cam_pose)

        color, depth = renderer.render(scene)

        # 保存颜色图
        img = Image.fromarray(color)
        img.save(os.path.join(args.out_dir, fname.replace(".obj", ".png")))

        # 输出并保存深度图以便诊断（如果深度全部为无穷或零，说明没有被相机看到）
        try:
            if depth is None:
                print(f"{fname}: depth is None")
            else:
                finite = np.isfinite(depth)
                if not finite.any():
                    print(f"{fname}: depth has no finite values (object not in view?)")
                else:
                    dmin = float(np.nanmin(depth[finite]))
                    dmax = float(np.nanmax(depth[finite]))
                    print(f"{fname}: depth min={dmin:.6f}, max={dmax:.6f}")
                    span = dmax - dmin
                    if span <= 0:
                        depth_img = (np.clip(depth, 0, 1) * 255).astype(np.uint8)
                    else:
                        norm = (depth - dmin) / span
                        norm[~finite] = 0.0
                        depth_img = (np.clip(norm, 0.0, 1.0) * 255).astype(np.uint8)
                    Image.fromarray(depth_img).save(
                        os.path.join(args.out_dir, fname.replace(".obj", "_depth.png"))
                    )
        except Exception as e:
            print(f"Failed to save depth for {fname}: {e}")

    print("Done.")


if __name__ == "__main__":
    main()
